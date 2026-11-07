/**

    @file      CrashHandler.h
    @brief     Writes a crash report (.txt and .dmp) to Documents\\LR10K crash reports when the app crashes.
    @details   Windows only. The text of the report is made by CrashReport.h (portable, unit tested).

               Caught: access violations and other Windows exceptions in any thread, uncaught C++ exceptions,
               abort() and std::terminate() (e.g. a std::thread destroyed while still running), pure virtual
               calls and invalid parameters to CRT functions.
               Not caught: crashes that Windows ends at once without asking the program (__fastfail, e.g.
               a /GS buffer-overrun check, some heap corruptions). Windows Error Reporting handles those.

               Nothing is installed while a debugger is attached, so Visual Studio still stops at the crash.
               To try it: start the app with --test-crash (outside Visual Studio, Ctrl+F5).

**/
#pragma once

#include <string>
#include <vector>

namespace CrashHandler
{
    //! Copies the last console lines into `out`; false if they could not be read (see CrashReport::RecentLines).
    using LinesProvider = bool (*)(std::vector<std::string>& out);

    /**
        @brief  Install the handlers. Call first thing in main().
        @param  appVersion - Shown in the report (copied).
        @param  lastLines  - The last console lines for the report (may be nullptr).
    **/
    void Install(const char* appVersion, LinesProvider lastLines);

    //! The folder the reports are written to (Documents\\LR10K crash reports). Empty if Documents was not found.
    std::wstring Folder();

    //! Opens the folder in Explorer (creates it if needed).
    void OpenFolder();

    /**
        @brief  If the last run crashed: the path of its .txt report. Returns it only once (the "new report"
                marker is removed), so the next start does not report the same crash again.
    **/
    std::wstring TakePreviousCrashReport();

    //! Crash on purpose (an access violation), to check that a report is written. Used by --test-crash.
    [[noreturn]] void TestCrash();
}
