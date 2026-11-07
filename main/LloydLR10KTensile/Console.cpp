/**

    @file      Console.cpp
    @brief     In-app console (log) window.
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"

#include "LR10KController.h"

// -----------------------------------------------------------------------------
// ------------------------------- Console System ------------------------------
// -----------------------------------------------------------------------------

void AppConsole::ClearLog()
{
    for (int i = 0; i < Items.Size; i++)
        free(Items[i]);
    Items.clear();
    std::lock_guard<std::mutex> lock(PendingMutex);
    Pending.clear();
}

void AppConsole::AddLog(const char* fmt, ...) IM_FMTARGS(2)
{
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);

    // One entry per line, so every entry has the same height (the drawing skips the lines that are not visible)
    std::lock_guard<std::mutex> lock(PendingMutex);
    const char* line = buf;
    while (true)
    {
        const char* end = strchr(line, '\n');
        Pending.emplace_back(line, end ? (size_t)(end - line) : strlen(line));
        Recent.Add(Pending.back());
        if (end == nullptr)
            break;
        line = end + 1;
    }
}

void AppConsole::FlushPending()
{
    std::vector<std::string> lines;
    {
        std::lock_guard<std::mutex> lock(PendingMutex);
        lines.swap(Pending);
    }
    if (lines.empty())
        return;

    for (const std::string& line : lines)
        Items.push_back(Strdup(line.c_str()));

    // Drop the oldest lines beyond the limit
    if (Items.Size > MaxItems)
    {
        const int drop = Items.Size - MaxItems;
        for (int i = 0; i < drop; i++)
            free(Items[i]);
        Items.erase(Items.begin(), Items.begin() + drop);
    }
}

void AppConsole::Draw(TabControl::TabTestDataClass* tabData, const char* title, bool* p_open, ImGuiWindowFlags extern_windowFlags)
{
    if (tabData == nullptr) return;
    FlushPending();

    ImGuiWindowFlags window_flags = 0;
    window_flags |= extern_windowFlags;


    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open, window_flags))
    {
        ImGui::End();
        return;
    }

    // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
    // So e.g. IsItemHovered() will return true when hovering the title bar.
    // Here we create a context menu only available from the title bar.
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Close Console"))
        {
            *p_open = false;
        }
        ImGui::EndPopup();
    }

    ImGui::TextWrapped("Basic console for the LR 10K Tensile Machine.");
    ImGui::Spacing();
    ImGui::TextWrapped("Enter 'HELP' for help. (like commands etc.)");

    // TODO: display items starting from the bottom

    /*if (ImGui::SmallButton("Add Debug Text")) { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
    ImGui::SameLine();*/
    if (ImGui::SmallButton("Clear")) { ClearLog(); }
    ImGui::SameLine();
    bool copy_to_clipboard = ImGui::SmallButton("Copy");
    //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

    ImGui::Separator();

    // Options menu
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    // Options, Filter
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushFont(Extra::fonts.jetbrainsFont);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }

        // Display every line as a separate entry so we can change their color or add custom widgets.
        // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
        // to only process visible items. The clipper will automatically measure the height of your first item and then
        // "seek" to display only items in the visible area.
        // To use the clipper we can replace your standard loop:
        //      for (int i = 0; i < Items.Size; i++)
        //   With:
        //      ImGuiListClipper clipper;
        //      clipper.Begin(Items.Size);
        //      while (clipper.Step())
        //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        // - That your items are evenly spaced (same height)
        // - That you have cheap random access to your elements (you can access them given their index,
        //   without processing all the ones before)
        // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
        // We would need random-access on the post-filtered list.
        // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
        // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
        // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
        // to improve this example code!
        // If your items are of variable height:
        // - Split them into same height items would be simpler and facilitate random-seeking into your list.
        // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (copy_to_clipboard)
            ImGui::LogToClipboard();
        // Only the visible lines are drawn (all lines are one text line high), unless a filter is active or the
        // log is copied. (Every line was drawn every frame: a long log slowed the whole GUI.)
        auto drawItem = [&](const char* item) {

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            else if (strstr(item, "[translation]")) { color = ImVec4(0.798f, 0.926f, 0.05f, 1.0f); has_color = true; }
            else if (strstr(item, "[before testing]")) { color = ImVec4(0.34f, 0.920f, 0.677f, 1.0f); has_color = true; }
            else if (strstr(item, "[testing]")) { color = ImVec4(0.044f, 0.920f, 0.677f, 1.0f); has_color = true; }
            else if (strstr(item, "[after testing]")) { color = ImVec4(0.34f, 0.920f, 0.677f, 1.0f); has_color = true; }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item);
            if (has_color)
                ImGui::PopStyleColor();
        };
        if (Filter.IsActive() || copy_to_clipboard)
        {
            for (int i = 0; i < Items.Size; i++)
                if (Filter.PassFilter(Items[i]))
                    drawItem(Items[i]);
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(Items.Size);
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    drawItem(Items[i]);
        }
        if (copy_to_clipboard)
            ImGui::LogFinish();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::Separator();

    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
    {
        char* s = InputBuf;
        Strtrim(s);
        if (s[0])
            ExecCommand(tabData, s);
        strcpy(s, "");
        reclaim_focus = true;
    }

    mainGUIGlobals::popUp_DimensionsNotAdded();

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

    ImGui::End();
}

void AppConsole::ExecCommand(TabControl::TabTestDataClass* tabData, const char* command_line)
{
    if (tabData == nullptr) return;

    AddLog("# %s\n", command_line);

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    HistoryPos = -1;
    for (int i = History.Size - 1; i >= 0; i--)
        if (Stricmp(History[i], command_line) == 0)
        {
            free(History[i]);
            History.erase(History.begin() + i);
            break;
        }
    History.push_back(Strdup(command_line));

    // Process command
    if (Stricmp(command_line, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(command_line, "HELP") == 0)
    {
        AddLog("Commands:");
        for (int i = 0; i < Commands.Size; i++)
            AddLog("- %s", Commands[i]);
    }
    else if (Stricmp(command_line, "HISTORY") == 0)
    {
        int first = History.Size - 10;
        for (int i = first > 0 ? first : 0; i < History.Size; i++)
            AddLog("%3d: %s\n", i, History[i]);
    }
    else if (!DockingApps::mainTabSystem.ThisTabIsNotRunningATest()) // Only allow control from one tab at the time
    {
        // BEFORE TESTING
        if (Stricmp(command_line, "ZERO") == 0)
        {
            if (LR10KControlSystem::SetZeroPoint(tabData))
                AddLog("[before testing] Setting zero point!");
            else
                AddLog("[error] Could not set zero point...");
        }

        // TESTING
        else if (Stricmp(command_line, "START") == 0)
        {
            if (LR10KControlSystem::StartTest(tabData))
                AddLog("[testing] Starting test!");
            else
                AddLog("[error] Could not start test...");
        }
        else if (Stricmp(command_line, "END") == 0)
        {
            if (LR10KControlSystem::EndTest(tabData))
                AddLog("[testing] Ended test!");
            else
                AddLog("[error] Could not end test...");
        }
        else if (Stricmp(command_line, "PAUSE") == 0)
        {
            if (LR10KControlSystem::PauseTest(tabData))
                AddLog("[testing] Pausing test!");
            else
                AddLog("[error] Could not pause test...");
        }
        else if (Stricmp(command_line, "CONTINUE") == 0)
        {
            if (LR10KControlSystem::ContinueTest(tabData))
                AddLog("[testing] Continue test!");
            else
                AddLog("[error] Could not continue test...");
        }

        // AFTER TESTING
        else if (Stricmp(command_line, "REZERO") == 0)
        {
            if (tabData->test_unfinished_test_started)
            {
                if (LR10KControlSystem::EndTest(tabData))
                    AddLog("[testing] Ended test!");
                else
                    AddLog("[error] Could not end test...");
            }

            if (LR10KControlSystem::ReturnToZeroPoint(tabData))
                AddLog("[after testing] Returning to zero point!");
            else
                AddLog("[error] Could not return to zero point...");
        }

        // TRANSLATION
        else if (Stricmp(command_line, "UP") == 0)
        {
            if (LR10KControlSystem::Translation_UP(tabData))
                AddLog("[translation] Moving UP");
            else
                AddLog("[error] Could not move up...");
        }
        else if (Stricmp(command_line, "FUP") == 0)
        {
            if (LR10KControlSystem::Translation_FASTUP_press(tabData))
                AddLog("[translation] Moving FAST UP");
            else
                AddLog("[error] Could not move fast up...");
        }
        else if (Stricmp(command_line, "DOWN") == 0)
        {
            if (LR10KControlSystem::Translation_DOWN(tabData))
                AddLog("[translation] Moving DOWN");
            else
                AddLog("[error] Could not move down...");
        }
        else if (Stricmp(command_line, "FDOWN") == 0)
        {
            if (LR10KControlSystem::Translation_FASTDOWN_press(tabData))
                AddLog("[translation] Moving FAST DOWN");
            else
                AddLog("[error] Could not move fast down...");
        }
        else if (Stricmp(command_line, "STOP") == 0)
        {
            if (LR10KControlSystem::Translation_STOP(tabData))
                AddLog("[translation] Movement STOPPED");
            else
                AddLog("[error] Could not stop...");
        }
    }
    else
    {
        AddLog("Unknown command: '%s'\n", command_line);
    }

    // On command input, we scroll to bottom even if AutoScroll==false
    ScrollToBottom = true;
}

int AppConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
    AppConsole* console = (AppConsole*)data->UserData;
    return console->TextEditCallback(data);
}

int AppConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Example of TEXT COMPLETION

        // Locate beginning of current word
        const char* word_end = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while (word_start > data->Buf)
        {
            const char c = word_start[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
                break;
            word_start--;
        }

        // Build a list of candidates
        ImVector<const char*> candidates;
        for (int i = 0; i < Commands.Size; i++)
            if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                candidates.push_back(Commands[i]);

        if (candidates.Size == 0)
        {
            // No match
            AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
        }
        else if (candidates.Size == 1)
        {
            // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
            data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
            data->InsertChars(data->CursorPos, candidates[0]);
            data->InsertChars(data->CursorPos, " ");
        }
        else
        {
            // Multiple matches. Complete as much as we can..
            // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
            int match_len = (int)(word_end - word_start);
            for (;;)
            {
                int c = 0;
                bool all_candidates_matches = true;
                for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                    if (i == 0)
                        c = toupper(candidates[i][match_len]);
                    else if (c == 0 || c != toupper(candidates[i][match_len]))
                        all_candidates_matches = false;
                if (!all_candidates_matches)
                    break;
                match_len++;
            }

            if (match_len > 0)
            {
                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
            }

            // List matches
            AddLog("Possible matches:\n");
            for (int i = 0; i < candidates.Size; i++)
                AddLog("- %s\n", candidates[i]);
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        // Example of HISTORY
        const int prev_history_pos = HistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (HistoryPos == -1)
                HistoryPos = History.Size - 1;
            else if (HistoryPos > 0)
                HistoryPos--;
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (HistoryPos != -1)
                if (++HistoryPos >= History.Size)
                    HistoryPos = -1;
        }

        // A better implementation would preserve the data on the current input line along with cursor position.
        if (prev_history_pos != HistoryPos)
        {
            const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}
