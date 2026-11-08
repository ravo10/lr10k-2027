/**

    @file      AutosaveApp.cpp
    @brief     Autosave of unsaved tests, and recovering them after a crash. See Autosave.h (portable, tested).
    @details   Everything here runs on the GUI thread, except the file writing (Autosave::Writer's thread).

**/

#include "common.h"
#include "Autosave.h"

#include <ShlObj.h>

#include <atomic>
#include <memory>

namespace
{
    std::filesystem::path g_folder;               //!< %LOCALAPPDATA%\LR10K\Autosave
    std::string g_session;                        //!< This run's part of the file names
    std::unique_ptr<Autosave::Writer> g_writer;
    Autosave::Scheduler g_scheduler;
    std::vector<std::filesystem::path> g_toOpen;  //!< Recovered files the user chose to open (one per frame)
    std::atomic<bool> g_failureReported{ false };

    std::filesystem::path AutosaveFolder()
    {
        std::filesystem::path folder;
        PWSTR localAppData = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &localAppData) == S_OK)
            folder = std::filesystem::path(localAppData) / L"LR10K" / L"Autosave";
        if (localAppData)
            CoTaskMemFree(localAppData);
        return folder;
    }

    std::filesystem::path FileOf(int tabId) { return g_folder / Autosave::FileName(g_session, tabId); }

    long long NowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    //! Changes when anything that is saved in the file changes.
    std::uint64_t ChangeKeyOf(const TabControl::TabTestDataClass& tab)
    {
        Autosave::ChangeKey key;
        const ImVector<ImVec2>& data = tab.LR10KTensileData.Data;
        key.Add(data.Size);
        if (data.Size > 0)
            key.Add(data[data.Size - 1].x).Add(data[data.Size - 1].y);
        for (const auto& point : tab.plot_dataPoints)
            key.Add(std::get<0>(point).x).Add(std::get<0>(point).y).Add(std::get<1>(point));
        key.Add(tab.thickness).Add(tab.width).Add(tab.length).Add(tab.diameter).Add(tab.area);
        key.Add(tab.description).Add(tab.unitsConvertionIndex).Add(tab.oldLoadErrorValue);
        key.Add(tab.areaOptionPicked).Add(tab.compressionOptionPicked).Add(tab.tension);
        key.Add(tab.test_finished).Add(tab.test_unfinished_test_started).Add(tab.test_running_test);
        return key.Value();
    }

    //! The time as the test date is written at the end of a test (ctime format, without its line break).
    std::string NowAsTestDate()
    {
        const std::time_t now = std::time(nullptr);
        char text[64] = {};
        if (ctime_s(text, sizeof(text), &now) != 0)
            return {};
        std::string date = text;
        while (!date.empty() && (date.back() == '\n' || date.back() == '\r'))
            date.pop_back();
        return date;
    }

    bool TestInProgress(const TabControl::TabTestDataClass& tab) { return tab.test_unfinished_test_started && !tab.test_finished; }

    //! Hand a copy of the tab to the writer thread, in the .lr10k format (the same as Save As).
    void WriteTab(TabControl::TabTestDataClass* tab)
    {
        TabControl::TabTestDataClass copy = *tab;
        tab->ConvertUnitsToNewUnit(&copy, 0, false, false, false); // Saved in mm and N, as every .lr10k file

        // A recovered test cannot be continued (the machine has moved on): it opens as a finished test
        if (TestInProgress(copy))
        {
            copy.test_finished = true;
            copy.test_unfinished_test_started = false;
            copy.test_running_test = false;
        }
        if (copy.date.empty())
            copy.date = NowAsTestDate();

        auto data = std::make_shared<json>(DockingApps::mainFileSystem.CreateJSONDataFromCurrentTab(&copy));
        g_writer->Write(FileOf((int)tab->tabId), [data] { return data->dump(); }); // Text made on the writer thread
    }

    struct Found
    {
        std::filesystem::path path;
        std::string summary; //!< For the message box
        bool readable = false;
    };

    std::vector<Found> FindFilesToRecover()
    {
        std::vector<std::string> names;
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(g_folder, error))
            if (entry.is_regular_file(error))
                names.push_back(entry.path().filename().string());

        std::vector<Found> found;
        for (const std::string& name : Autosave::FilesToRecover(names, g_session))
        {
            Found f;
            f.path = g_folder / name;
            try
            {
                std::ifstream in(f.path);
                const json test = json::parse(in);
                const std::size_t points = test.contains("LR10KTensileData") ? test["LR10KTensileData"].size() : 0;
                std::string description = test.value("description", std::string());
                if (description.size() > 40)
                    description = description.substr(0, 40) + "...";
                f.summary = test.value("date", std::string("(no date)")) + ", " + std::to_string(points) + " points"
                    + (description.empty() ? std::string() : ", \"" + description + "\"");
                f.readable = true;
            }
            catch (...)
            {
                f.summary = name + " (damaged, cannot be opened)";
            }
            found.push_back(std::move(f));
        }
        return found;
    }

    std::wstring Wide(const std::string& text)
    {
        if (text.empty()) return {};
        const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);
        std::wstring out(size > 0 ? size : 0, L'\0');
        if (size > 0)
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), out.data(), size);
        return out;
    }
}

void AppAutosave::Start()
{
    g_folder = AutosaveFolder();
    if (g_folder.empty())
    {
        DockingApps::console.AddLog("[error] Autosave is off: the AppData folder was not found.");
        return;
    }
    std::error_code error;
    std::filesystem::create_directories(g_folder, error);

    SYSTEMTIME now{};
    GetLocalTime(&now);
    g_session = Autosave::SessionStamp(now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, GetCurrentProcessId());

    g_writer = std::make_unique<Autosave::Writer>([](const std::filesystem::path& path, bool ok) {
        // (Writer thread; the console is thread-safe.) Report a failure once, not every few seconds.
        if (!ok && !g_failureReported.exchange(true))
            DockingApps::console.AddLog("[error] Autosave could not write %s. Save the test yourself (Ctrl+S).", path.string().c_str());
        if (ok)
            g_failureReported = false;
    });
}

void AppAutosave::AskToRecover()
{
    if (g_folder.empty())
        return;
    const std::vector<Found> found = FindFilesToRecover();
    if (found.empty())
        return;

    std::string list;
    bool anyReadable = false;
    for (const Found& f : found)
    {
        list += "    - " + f.summary + "\n";
        anyReadable |= f.readable;
    }
    const std::string message = "The LR 10K software did not close normally last time. These tests were not saved, "
        "but the autosave kept them:\n\n" + list + "\n"
        "Yes: open them as new tabs (then save them).\n"
        "No: delete them.\n"
        "Cancel: decide the next time the software starts.";
    const int answer = MessageBoxW(NULL, Wide(message).c_str(), L"Recover unsaved tests", MB_YESNOCANCEL | MB_ICONWARNING);

    std::error_code error;
    if (answer == IDYES)
    {
        for (const Found& f : found)
        {
            if (f.readable)
                g_toOpen.push_back(f.path);
            else
            {
                // Keep it for a look, but do not ask about it again
                std::filesystem::path damaged = f.path;
                damaged += ".damaged";
                std::filesystem::rename(f.path, damaged, error);
            }
        }
        if (!anyReadable)
            DockingApps::console.AddLog("[error] The autosaved tests were damaged and could not be opened (kept as .damaged in %s).", g_folder.string().c_str());
    }
    else if (answer == IDNO)
    {
        for (const Found& f : found)
            std::filesystem::remove(f.path, error);
        DockingApps::console.AddLog("Deleted %d autosaved test(s) from the last session.", (int)found.size());
    }
}

void AppAutosave::Tick()
{
    if (!g_writer)
        return;

    // 1. Open one recovered test per frame
    if (!g_toOpen.empty())
    {
        const std::filesystem::path path = g_toOpen.front();
        g_toOpen.erase(g_toOpen.begin());
        TabControl::TabTestDataClass* tab = DockingApps::mainFileSystem.OpenTestFile(path.string());
        if (tab != nullptr)
        {
            // Not a file of the user's: Save must ask where to save it, and the tab is unsaved
            tab->loadedFromFileName = "";
            tab->current_temp_preSaveLocation = "";
            tab->MarkTabAsUnsaved();
            DockingApps::console.AddLog("[warning] Recovered an unsaved test (%s). Save it with Save As (Ctrl+Alt+S).", tab->date.c_str());
            // This session's autosave takes over (written at once, the tab is unsaved); the old file can go
            g_writer->Remove(path);
        }
    }

    // 2. Autosave the tabs that need it, delete the files of tabs that were saved or closed
    std::vector<Autosave::TabState> tabs;
    // Only open tabs: a closed tab stays in allTabTestData (only its ID leaves activeTestTabsId), and its
    // autosave must go when it is closed, also when the user closed it without saving
    for (int i = 0; i < TabControl::activeTestTabsId.Size; i++)
    {
        TabControl::TabTestDataClass* tab = TabControl::FindTabTestData(TabControl::activeTestTabsId[i]);
        if (tab == nullptr)
            continue;
        Autosave::TabState state;
        state.tabId = (int)tab->tabId;
        state.testRunning = TestInProgress(*tab);
        state.needsAutosave = tab->LR10KTensileData.Data.Size > 0 && (tab->unsavedThingsInTab || state.testRunning);
        state.changeKey = state.needsAutosave ? ChangeKeyOf(*tab) : 0;
        tabs.push_back(state);
    }
    const Autosave::Plan plan = g_scheduler.Update(tabs, NowMs());
    for (int tabId : plan.save)
        if (TabControl::TabTestDataClass* tab = TabControl::FindTabTestData(tabId))
            WriteTab(tab);
    for (int tabId : plan.remove)
        g_writer->Remove(FileOf(tabId));
}

void AppAutosave::Shutdown()
{
    if (!g_writer)
        return;
    // A normal exit (the user was asked about unsaved tabs): this session's files are not needed any more
    for (int tabId : g_scheduler.TabsWithFiles())
        g_writer->Remove(FileOf(tabId));
    g_writer->Flush();
    g_writer->Stop();
    g_writer.reset();
}

std::wstring AppAutosave::Folder()
{
    return g_folder.wstring();
}
