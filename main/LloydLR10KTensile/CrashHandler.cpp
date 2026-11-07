/**

    @file      CrashHandler.cpp
    @brief     Catches crashes and writes the report. See CrashHandler.h.
    @details   A crashed process is in an unknown state, so the handler:
               - only writes the first crash (a second thread that crashes meanwhile waits);
               - frees a reserved block of memory first (in case the crash was "out of memory");
               - does the work on a new thread, so it has a fresh stack (needed after a stack overflow);
               - writes the minidump first, then the text: if the text fails, the dump is still there;
               - reads the console lines with a time limit (the crashed thread may hold their lock).

**/

#include "CrashHandler.h"
#include "CrashReport.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <ShlObj.h>
#include <shellapi.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>

#pragma comment(lib, "dbghelp.lib")

namespace
{
    std::wstring g_folder;
    std::string g_version;
    CrashHandler::LinesProvider g_lastLines = nullptr;
    DWORD g_mainThreadId = 0;
    std::atomic<bool> g_handling{ false };
    void* g_reserve = nullptr;            //!< Freed at a crash, so the report can be written when memory ran out
    const char* g_terminateWhat = nullptr; //!< Set by the terminate handler, read by the abort handler

    constexpr std::size_t kReserveBytes = 2 * 1024 * 1024;
    constexpr std::size_t kKeepReports = 10;
    constexpr int kMaxFrames = 48;

    std::string Utf8(const std::wstring& text)
    {
        if (text.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
        std::string out(size > 0 ? size : 0, '\0');
        if (size > 0)
            WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), out.data(), size, nullptr, nullptr);
        return out;
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

    bool WriteTextFile(const std::wstring& path, const std::string& text)
    {
        HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0;
        const BOOL ok = WriteFile(file, text.data(), (DWORD)text.size(), &written, nullptr);
        CloseHandle(file);
        return ok && written == text.size();
    }

    std::string ReadTextFile(const std::wstring& path)
    {
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return {};
        char buffer[512];
        DWORD read = 0;
        std::string text;
        if (ReadFile(file, buffer, sizeof(buffer), &read, nullptr))
            text.assign(buffer, read);
        CloseHandle(file);
        return text;
    }

    std::wstring ModuleName(DWORD64 base)
    {
        wchar_t path[MAX_PATH] = {};
        if (base == 0 || GetModuleFileNameW((HMODULE)base, path, MAX_PATH) == 0)
            return {};
        const wchar_t* slash = wcsrchr(path, L'\\');
        return slash ? slash + 1 : path;
    }

    //! The call stack of `thread`, starting at `context`. Function names and lines need the .pdb next to the .exe.
    std::vector<CrashReport::Frame> CallStack(HANDLE thread, CONTEXT context)
    {
        std::vector<CrashReport::Frame> frames;
        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS);
        const bool symbols = SymInitialize(process, nullptr, TRUE) != FALSE;

        STACKFRAME64 frame{};
#if defined(_M_X64) || defined(__x86_64__)
        const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
        frame.AddrPC.Offset = context.Rip;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrStack.Offset = context.Rsp;
#else
        const DWORD machine = IMAGE_FILE_MACHINE_I386;
        frame.AddrPC.Offset = context.Eip;
        frame.AddrFrame.Offset = context.Ebp;
        frame.AddrStack.Offset = context.Esp;
#endif
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;

        while ((int)frames.size() < kMaxFrames
            && StackWalk64(machine, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
        {
            const DWORD64 address = frame.AddrPC.Offset;
            if (address == 0)
                break;

            CrashReport::Frame f;
            const DWORD64 base = SymGetModuleBase64(process, address);
            f.module = Utf8(ModuleName(base));
            f.offset = base ? address - base : address;

            if (symbols)
            {
                alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256] = {};
                SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = 255;
                DWORD64 displacement = 0;
                if (SymFromAddr(process, address, &displacement, symbol))
                    f.function = symbol->Name;

                IMAGEHLP_LINE64 line{};
                line.SizeOfStruct = sizeof(line);
                DWORD lineDisplacement = 0;
                if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
                {
                    f.file = line.FileName ? line.FileName : "";
                    f.line = (int)line.LineNumber;
                }
            }
            frames.push_back(std::move(f));
        }
        if (symbols)
            SymCleanup(process);
        return frames;
    }

    //! Keep only the newest reports.
    void DeleteOldReports()
    {
        std::vector<std::string> stems;
        WIN32_FIND_DATAW found{};
        HANDLE find = FindFirstFileW((g_folder + L"\\crash_*.txt").c_str(), &found);
        if (find == INVALID_HANDLE_VALUE)
            return;
        do
        {
            std::wstring name = found.cFileName;
            if (name.size() > 4)
                stems.push_back(Utf8(name.substr(0, name.size() - 4)));
        } while (FindNextFileW(find, &found));
        FindClose(find);

        for (const std::string& stem : CrashReport::OldReports(stems, kKeepReports))
        {
            DeleteFileW((g_folder + L"\\" + Wide(stem) + L".txt").c_str());
            DeleteFileW((g_folder + L"\\" + Wide(stem) + L".dmp").c_str());
        }
    }

    //! What one crash needs to be written (filled by the crashed thread, used by the writer thread).
    struct Job
    {
        EXCEPTION_POINTERS* exception = nullptr; //!< nullptr: no exception record (e.g. a pure virtual call)
        CONTEXT context{};                      //!< Where the stack walk starts
        DWORD threadId = 0;
        HANDLE thread = nullptr;                //!< The crashed thread (a real handle, usable from another thread)
        std::string what;
        DWORD code = 0;
        bool gotCode = false;
    };

    void WriteReport(Job& job)
    {
        if (g_folder.empty())
            return;
        CreateDirectoryW(g_folder.c_str(), nullptr);

        SYSTEMTIME now{};
        GetLocalTime(&now);
        const std::string stem = CrashReport::FileStem(now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
        const std::wstring base = g_folder + L"\\" + Wide(stem);

        CrashReport::Info info;
        info.appVersion = g_version;
        char time[64];
        snprintf(time, sizeof(time), "%02d.%02d.%04d %02d:%02d:%02d", now.wDay, now.wMonth, now.wYear, now.wHour, now.wMinute, now.wSecond);
        info.time = time;
        info.what = job.what;
        info.code = job.code;
        info.gotCode = job.gotCode;
        info.thread = job.threadId == g_mainThreadId ? std::string("GUI (main)") : "other (id " + std::to_string(job.threadId) + ")";

        // 1. The minidump: the most useful file, so first
        HANDLE dump = CreateFileW((base + L".dmp").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (dump != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
            exceptionInfo.ThreadId = job.threadId;
            exceptionInfo.ExceptionPointers = job.exception;
            exceptionInfo.ClientPointers = FALSE;
            const MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory
                | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
            if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump, type,
                job.exception ? &exceptionInfo : nullptr, nullptr, nullptr))
                info.dumpFile = stem + ".dmp";
            CloseHandle(dump);
            if (info.dumpFile.empty())
                DeleteFileW((base + L".dmp").c_str());
        }

        // 2. The text: where, and what the user was doing
        info.stack = CallStack(job.thread, job.context);
        if (g_lastLines)
            info.gotLastLines = g_lastLines(info.lastLines);
        WriteTextFile(base + L".txt", CrashReport::Format(info));

        // 3. Tell the next start about it, and keep the folder small
        WriteTextFile(g_folder + L"\\new-report.txt", stem);
        DeleteOldReports();
    }

    DWORD WINAPI WriterThread(LPVOID parameter)
    {
        WriteReport(*static_cast<Job*>(parameter));
        return 0;
    }

    //! Write the report (once per process), then end the process without the Windows error dialog.
    [[noreturn]] void HandleCrash(Job& job)
    {
        if (g_handling.exchange(true))
        {
            // Another thread is writing a report already; it ends the process when done
            Sleep(30000);
            TerminateProcess(GetCurrentProcess(), 3);
        }
        if (g_reserve)
        {
            VirtualFree(g_reserve, 0, MEM_RELEASE);
            g_reserve = nullptr;
        }

        job.threadId = GetCurrentThreadId();
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &job.thread, 0, FALSE, DUPLICATE_SAME_ACCESS);

        // A new thread has a fresh stack (the crashed thread's may be used up); write here if it cannot start
        HANDLE writer = CreateThread(nullptr, 1024 * 1024, WriterThread, &job, 0, nullptr);
        if (writer)
        {
            WaitForSingleObject(writer, 60000);
            CloseHandle(writer);
        }
        else
            WriteReport(job);

        TerminateProcess(GetCurrentProcess(), 3);
        for (;;) Sleep(INFINITE); // TerminateProcess does not return for the own process
    }

    //! A crash without an exception record: start the stack walk here.
    [[noreturn]] void HandleCrashHere(const char* what)
    {
        Job job;
        RtlCaptureContext(&job.context);
        job.what = what;
        HandleCrash(job);
    }

    LONG WINAPI UnhandledException(EXCEPTION_POINTERS* exception)
    {
        Job job;
        job.exception = exception;
        job.context = *exception->ContextRecord;
        job.code = exception->ExceptionRecord->ExceptionCode;
        job.gotCode = true;
        job.what = CrashReport::ExceptionName(job.code);
        if (job.code == 0xC0000005u && exception->ExceptionRecord->NumberParameters >= 2)
        {
            // Access violation: reading or writing, and at which address (near 0: a null pointer)
            const ULONG_PTR kind = exception->ExceptionRecord->ExceptionInformation[0];
            const ULONG_PTR address = exception->ExceptionRecord->ExceptionInformation[1];
            char detail[96];
            snprintf(detail, sizeof(detail), ", %s address 0x%llx%s", kind == 1 ? "writing to" : kind == 8 ? "executing" : "reading from",
                (unsigned long long)address, address < 0x10000 ? " (a null pointer)" : "");
            job.what += detail;
        }
        HandleCrash(job);
    }

    void AbortSignal(int)
    {
        HandleCrashHere(g_terminateWhat ? g_terminateWhat
            : "abort() was called (e.g. an uncaught C++ exception or std::terminate() in a worker thread, or a failed check)");
    }

    void Terminate()
    {
        // Main thread only (the terminate handler is per thread); worker threads end up in AbortSignal
        static std::string what;
        what = "std::terminate() was called";
        if (std::exception_ptr current = std::current_exception())
        {
            try { std::rethrow_exception(current); }
            catch (const std::exception& e) { what = std::string("Uncaught C++ exception: ") + e.what(); }
            catch (...) { what = "Uncaught C++ exception (not a std::exception)"; }
        }
        else
            what += " (e.g. a std::thread was destroyed while still running)";
        g_terminateWhat = what.c_str();
        HandleCrashHere(g_terminateWhat);
    }

    void PureCall() { HandleCrashHere("Pure virtual function call (an object was used during or after its destruction)"); }

    void InvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
    {
        HandleCrashHere("Invalid parameter passed to a C runtime function (e.g. a null pointer or a bad format string)");
    }
}

void CrashHandler::Install(const char* appVersion, LinesProvider lastLines)
{
    g_version = appVersion ? appVersion : "";
    g_lastLines = lastLines;
    g_mainThreadId = GetCurrentThreadId();
    g_folder = Folder();

    if (IsDebuggerPresent())
        return; // Let Visual Studio stop at the crash

    g_reserve = VirtualAlloc(nullptr, kReserveBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    // Room on the main thread's stack for the handler after a stack overflow
    ULONG stackGuarantee = 64 * 1024;
    SetThreadStackGuarantee(&stackGuarantee);

    SetUnhandledExceptionFilter(UnhandledException);
    std::set_terminate(Terminate);
    std::signal(SIGABRT, AbortSignal);
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_purecall_handler(PureCall);
    _set_invalid_parameter_handler(InvalidParameter);
#else
    (void)PureCall;
    (void)InvalidParameter;
#endif
}

std::wstring CrashHandler::Folder()
{
    std::wstring folder;
    PWSTR documents = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, NULL, &documents) == S_OK)
        folder = std::wstring(documents) + L"\\LR10K crash reports";
    if (documents)
        CoTaskMemFree(documents);
    return folder;
}

void CrashHandler::OpenFolder()
{
    const std::wstring folder = Folder();
    if (folder.empty())
        return;
    CreateDirectoryW(folder.c_str(), nullptr);
    ShellExecuteW(NULL, L"open", folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

std::wstring CrashHandler::TakePreviousCrashReport()
{
    const std::wstring folder = Folder();
    if (folder.empty())
        return {};
    const std::wstring marker = folder + L"\\new-report.txt";
    std::string stem = ReadTextFile(marker);
    if (stem.empty())
        return {};
    DeleteFileW(marker.c_str());

    while (!stem.empty() && (stem.back() == '\n' || stem.back() == '\r' || stem.back() == ' '))
        stem.pop_back();
    const std::wstring report = folder + L"\\" + Wide(stem) + L".txt";
    return GetFileAttributesW(report.c_str()) != INVALID_FILE_ATTRIBUTES ? report : std::wstring();
}

void CrashHandler::TestCrash()
{
    volatile int* nothing = nullptr;
    *nothing = 1;
    std::abort(); // Not reached
}
