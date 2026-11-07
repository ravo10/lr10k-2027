/**

    @file      CrashReport.h
    @brief     The text of a crash report, and the last console lines that go into it.
    @details   Portable (no Windows code) and unit tested (tests/crash_report_test.cpp).
               The Windows part (catching the crash, the .dmp file, the call stack) is in CrashHandler.cpp.

               When the app crashes on the lab PC, two files are written to Documents\\LR10K crash reports:
               - crash_<date>_<time>.txt: what happened, where (function, file and line if the .pdb is there),
                 and the last console lines (what the user and the machine were doing);
               - crash_<date>_<time>.dmp: a minidump. Open it in Visual Studio (with the .pdb from the same
                 build) to see the call stack and the variables of every thread at the moment of the crash.
               The next start of the app says that a report was written, and where.

**/
#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace CrashReport
{
    //! A readable name for a Windows exception code (the codes are fixed by Windows, so no Windows header is needed).
    inline const char* ExceptionName(std::uint32_t code)
    {
        switch (code)
        {
        case 0xC0000005u: return "Access violation (a bad pointer: null, deleted or out of range)";
        case 0xC00000FDu: return "Stack overflow (endless recursion or a very large local array)";
        case 0xC0000094u: return "Integer division by zero";
        case 0xC0000095u: return "Integer overflow";
        case 0xC000008Cu: return "Array bounds exceeded";
        case 0xC000008Eu: return "Floating-point division by zero";
        case 0xC0000090u: return "Floating-point invalid operation";
        case 0xC000001Du: return "Illegal instruction";
        case 0xC0000006u: return "In-page error (the program file could not be read, e.g. a network drive or USB stick was removed)";
        case 0xC0000409u: return "Stack buffer overrun (memory was written past the end of a local array)";
        case 0xC0000374u: return "Heap corruption (memory was written past the end of an allocation, or freed twice)";
        case 0xC0000017u: return "Out of memory";
        case 0x80000003u: return "Breakpoint";
        case 0xE06D7363u: return "Uncaught C++ exception";
        default:          return "Unknown exception";
        }
    }

    //! "crash_2026-11-07_22-38-55": sorts by time, and is the name of both files.
    inline std::string FileStem(int year, int month, int day, int hour, int minute, int second)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "crash_%04d-%02d-%02d_%02d-%02d-%02d", year, month, day, hour, minute, second);
        return text;
    }

    /**
        @brief  The reports to delete so only the newest `keep` remain.
        @param  stems - The file stems (FileStem()) of the reports in the folder, in any order.
    **/
    inline std::vector<std::string> OldReports(std::vector<std::string> stems, std::size_t keep)
    {
        std::sort(stems.begin(), stems.end()); // Oldest first (the stem is the time)
        if (stems.size() <= keep)
            return {};
        stems.resize(stems.size() - keep);
        return stems;
    }

    //! One frame of the call stack.
    struct Frame
    {
        std::string function;  //!< Empty if unknown (no .pdb)
        std::string file;      //!< Empty if unknown
        int line = 0;
        std::string module;    //!< e.g. "Lloyd LR10K Tensile Machine Control Software.exe"
        std::uint64_t offset = 0; //!< Address minus the module's base (find it in the .pdb with the same build)
    };

    //! Everything that goes into the .txt file.
    struct Info
    {
        std::string appVersion;
        std::string time;          //!< Local time, readable
        std::string what;          //!< e.g. ExceptionName(), "abort() was called", "Uncaught C++ exception: ..."
        std::uint32_t code = 0;    //!< Windows exception code (0: none, e.g. abort)
        bool gotCode = false;
        std::string thread;        //!< "GUI (main)" or "other (id ...)"
        std::vector<Frame> stack;
        std::string dumpFile;      //!< Empty if the minidump could not be written
        std::vector<std::string> lastLines;
        bool gotLastLines = true;  //!< False if the console was busy (another thread held it when the crash happened)
    };

    inline std::string FrameText(const Frame& f)
    {
        std::string text = f.function.empty() ? "?" : f.function;
        if (!f.file.empty())
        {
            // Only the file name: the build folder of the PC it was built on means nothing to the reader
            const std::size_t slash = f.file.find_last_of("/\\");
            text += "  (" + (slash == std::string::npos ? f.file : f.file.substr(slash + 1)) + ":" + std::to_string(f.line) + ")";
        }
        char where[64];
        std::snprintf(where, sizeof(where), "+0x%llx", (unsigned long long)f.offset);
        text += "  [" + (f.module.empty() ? std::string("?") : f.module) + where + "]";
        return text;
    }

    //! The text of the .txt file.
    inline std::string Format(const Info& info)
    {
        std::string text;
        text += "LR 10K Control Software crashed\n";
        text += "===============================\n\n";
        text += "Version: " + info.appVersion + "\n";
        text += "Time:    " + info.time + "\n";
        text += "What:    " + info.what + "\n";
        if (info.gotCode)
        {
            char code[32];
            std::snprintf(code, sizeof(code), "0x%08X", (unsigned)info.code);
            text += "Code:    " + std::string(code) + "\n";
        }
        text += "Thread:  " + info.thread + "\n";
        text += "Dump:    " + (info.dumpFile.empty() ? std::string("(could not be written)") : info.dumpFile) + "\n";

        text += "\nCall stack (innermost first):\n";
        if (info.stack.empty())
            text += "  (not available)\n";
        for (std::size_t i = 0; i < info.stack.size(); ++i)
            text += "  #" + std::to_string(i) + "  " + FrameText(info.stack[i]) + "\n";

        text += "\nLast console lines:\n";
        if (!info.gotLastLines)
            text += "  (not available: the console was in use when the crash happened)\n";
        else if (info.lastLines.empty())
            text += "  (none)\n";
        for (const std::string& line : info.lastLines)
            text += "  " + line + "\n";

        text += "\nPlease send this file and the .dmp file with the same name to the developer,\n";
        text += "with a few words on what you were doing.\n";
        return text;
    }

    /**
        @brief  The last console lines, kept apart so a crash report can read them from any thread.
        @details Add() is called for every console line (from any thread). TryCopy() is called by the crash
                 handler: it does not wait forever for the lock, because the thread that crashed may hold it.
    **/
    class RecentLines
    {
    public:
        explicit RecentLines(std::size_t capacity = 200) : m_capacity(capacity) {}

        void Add(std::string line)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lines.push_back(std::move(line));
            while (m_lines.size() > m_capacity)
                m_lines.pop_front();
        }

        //! @return false if the lock could not be taken within `wait` (the lines are then not copied).
        bool TryCopy(std::vector<std::string>& out, std::chrono::milliseconds wait = std::chrono::milliseconds(200))
        {
            const auto until = std::chrono::steady_clock::now() + wait;
            while (!m_mutex.try_lock())
            {
                if (std::chrono::steady_clock::now() >= until)
                    return false;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            out.assign(m_lines.begin(), m_lines.end());
            m_mutex.unlock();
            return true;
        }

        //! Holds the lock (for tests: a thread that crashed while adding a line).
        std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(m_mutex); }

    private:
        std::size_t m_capacity;
        std::mutex m_mutex;
        std::deque<std::string> m_lines;
    };
}
