/**

    @file      Autosave.h
    @brief     Autosave of unsaved tests: when to write, the file names, and a writer thread.
    @details   Portable (no Windows, ImGui or JSON code) and unit tested (tests/autosave_test.cpp).
               The app side (which tabs, the .lr10k JSON, recovering at start) is in FileSystem.cpp.

               While a tab has data that is not saved (a test running or paused, or a finished test not saved
               yet), a copy is written to %LOCALAPPDATA%\\LR10K\\Autosave every few seconds:
               - autosave_<session>_tab<id>.lr10k, <session> = start time and process id of this run;
               - the file of a tab is deleted when the tab is saved or closed, and all of this session's files
                 at a normal exit;
               - so files of another session left in the folder mean that run did not end normally (a crash,
                 a power cut, Windows ended it): the next start offers to open them.

               Files are written to a .tmp file first and then renamed, so a crash while writing never leaves a
               half-written file. Writing runs on its own thread, so a large test does not make the GUI stutter.

**/
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Autosave
{
    constexpr long long kIntervalDuringTestMs = 5000;  //!< While a test runs (the data grows all the time)
    constexpr long long kIntervalOtherwiseMs = 15000;  //!< Unsaved changes after a test (points moved, dimensions, ...)

    //! "20261108-101502-4242": start time and process id, so two running copies of the app do not mix their files.
    inline std::string SessionStamp(int year, int month, int day, int hour, int minute, int second, unsigned long processId)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "%04d%02d%02d-%02d%02d%02d-%lu", year, month, day, hour, minute, second, processId);
        return text;
    }

    inline std::string FileName(const std::string& session, int tabId)
    {
        return "autosave_" + session + "_tab" + std::to_string(tabId) + ".lr10k";
    }

    //! Splits a file name made by FileName(); false for any other file.
    inline bool ParseFileName(const std::string& name, std::string& session, int& tabId)
    {
        const std::string prefix = "autosave_", suffix = ".lr10k";
        if (name.size() <= prefix.size() + suffix.size() || name.compare(0, prefix.size(), prefix) != 0
            || name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0)
            return false;
        const std::string middle = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
        const std::size_t tab = middle.rfind("_tab");
        if (tab == std::string::npos || tab == 0 || tab + 4 >= middle.size())
            return false;
        const std::string number = middle.substr(tab + 4);
        if (number.find_first_not_of("0123456789") != std::string::npos || number.size() > 9)
            return false;
        session = middle.substr(0, tab);
        tabId = std::stoi(number);
        return true;
    }

    //! The autosave files in `names` that belong to another session (= to recover).
    inline std::vector<std::string> FilesToRecover(const std::vector<std::string>& names, const std::string& currentSession)
    {
        std::vector<std::string> files;
        for (const std::string& name : names)
        {
            std::string session;
            int tabId = 0;
            if (ParseFileName(name, session, tabId) && session != currentSession)
                files.push_back(name);
        }
        return files;
    }

    //! A small hash to tell whether a tab changed since its last autosave (FNV-1a).
    class ChangeKey
    {
    public:
        template <class T>
        ChangeKey& Add(const T& value)
        {
            const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
            for (std::size_t i = 0; i < sizeof(T); ++i)
                m_hash = (m_hash ^ bytes[i]) * 1099511628211ull;
            return *this;
        }
        ChangeKey& Add(const std::string& text)
        {
            for (unsigned char c : text)
                m_hash = (m_hash ^ c) * 1099511628211ull;
            return Add(text.size());
        }
        std::uint64_t Value() const { return m_hash; }

    private:
        std::uint64_t m_hash = 1469598103934665603ull;
    };

    //! A tab as the scheduler sees it.
    struct TabState
    {
        int tabId = 0;
        bool needsAutosave = false;  //!< Has data that is not saved
        bool testRunning = false;    //!< A test is running or paused: the shorter interval
        std::uint64_t changeKey = 0;
    };

    //! What to do this frame.
    struct Plan
    {
        std::vector<int> save;   //!< Write these tabs now
        std::vector<int> remove; //!< Delete the files of these tabs (saved, closed, or nothing to keep)
    };

    /**
        @brief  Decides when each tab is written, and when its file goes away.
        @details A tab is written when it needs an autosave, changed since its last one, and its interval has
                 passed (the first time: at once). Its file is removed when it no longer needs one or is gone.
    **/
    class Scheduler
    {
    public:
        Plan Update(const std::vector<TabState>& tabs, long long nowMs)
        {
            Plan plan;
            std::map<int, Entry> next;
            for (const TabState& tab : tabs)
            {
                auto found = m_written.find(tab.tabId);
                if (!tab.needsAutosave)
                {
                    if (found != m_written.end())
                        plan.remove.push_back(tab.tabId); // Saved now (or emptied): its autosave is not needed
                    continue;
                }
                if (found == m_written.end())
                {
                    plan.save.push_back(tab.tabId);
                    next[tab.tabId] = Entry{ nowMs, tab.changeKey };
                    continue;
                }
                const long long interval = tab.testRunning ? kIntervalDuringTestMs : kIntervalOtherwiseMs;
                Entry entry = found->second;
                if (tab.changeKey != entry.changeKey && nowMs - entry.lastMs >= interval)
                {
                    plan.save.push_back(tab.tabId);
                    entry = Entry{ nowMs, tab.changeKey };
                }
                next[tab.tabId] = entry;
            }
            // Tabs that were closed
            for (const auto& [tabId, entry] : m_written)
                if (next.find(tabId) == next.end())
                {
                    bool stillThere = false;
                    for (const TabState& tab : tabs)
                        stillThere |= tab.tabId == tabId;
                    if (!stillThere)
                        plan.remove.push_back(tabId);
                }
            m_written.swap(next);
            return plan;
        }

        //! All tabs with an autosave file now (for the clean-up at exit).
        std::vector<int> TabsWithFiles() const
        {
            std::vector<int> ids;
            for (const auto& item : m_written)
                ids.push_back(item.first);
            return ids;
        }

    private:
        struct Entry
        {
            long long lastMs = 0;
            std::uint64_t changeKey = 0;
        };
        std::map<int, Entry> m_written;
    };

    /**
        @brief  Writes `content` to `path` through `path`.tmp and a rename, so `path` is always either the old or
                the whole new file.
    **/
    inline bool WriteFileAtomically(const std::filesystem::path& path, const std::string& content)
    {
        std::filesystem::path temporary = path;
        temporary += ".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out)
                return false;
            out.write(content.data(), (std::streamsize)content.size());
            out.flush();
            if (!out)
                return false;
        }
        std::error_code error;
        std::filesystem::rename(temporary, path, error); // Replaces an existing file
        if (error)
        {
            std::filesystem::remove(temporary, error);
            return false;
        }
        return true;
    }

    /**
        @brief  A thread that writes and deletes autosave files, so the GUI thread only hands over the work.
        @details For each file only the newest request counts: a write that is still waiting is replaced by a newer
                 write or by a delete. The content is made on the writer thread (the producer function), so the
                 slow part (turning the data into text) does not run on the GUI thread either.
    **/
    class Writer
    {
    public:
        using Producer = std::function<std::string()>;
        using Report = std::function<void(const std::filesystem::path&, bool ok)>; //!< Called on the writer thread after a write

        explicit Writer(Report report = {}) : m_report(std::move(report)), m_thread([this] { Run(); }) {}
        ~Writer() { Stop(); }
        Writer(const Writer&) = delete;
        Writer& operator=(const Writer&) = delete;

        void Write(const std::filesystem::path& path, Producer producer)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pending[path.string()] = Job{ path, std::move(producer), false };
            }
            m_wake.notify_one();
        }

        void Remove(const std::filesystem::path& path)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pending[path.string()] = Job{ path, {}, true };
            }
            m_wake.notify_one();
        }

        //! Waits until everything handed over is done (for the clean-up at exit, and for tests).
        void Flush()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_idle.wait(lock, [this] { return m_pending.empty() && !m_busy; });
        }

        //! Finishes the waiting work, then ends the thread.
        void Stop()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_stop)
                    return;
                m_stop = true;
            }
            m_wake.notify_one();
            if (m_thread.joinable())
                m_thread.join();
        }

    private:
        struct Job
        {
            std::filesystem::path path;
            Producer producer;
            bool remove = false;
        };

        void Run()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            for (;;)
            {
                m_wake.wait(lock, [this] { return m_stop || !m_pending.empty(); });
                if (m_pending.empty())
                {
                    if (m_stop)
                        return;
                    continue;
                }
                Job job = std::move(m_pending.begin()->second);
                m_pending.erase(m_pending.begin());
                m_busy = true;
                lock.unlock();

                if (job.remove)
                {
                    std::error_code error;
                    std::filesystem::remove(job.path, error);
                }
                else
                {
                    bool ok = false;
                    try { ok = WriteFileAtomically(job.path, job.producer ? job.producer() : std::string()); }
                    catch (...) { ok = false; } // A failed autosave must never take the app down
                    if (m_report)
                        m_report(job.path, ok);
                }

                lock.lock();
                m_busy = false;
                if (m_pending.empty())
                    m_idle.notify_all();
            }
        }

        Report m_report;
        std::mutex m_mutex;
        std::condition_variable m_wake, m_idle;
        std::map<std::string, Job> m_pending;
        bool m_busy = false;
        bool m_stop = false;
        std::thread m_thread; // Last: starts after the members above exist
    };
}
