// Unit tests for CrashReport (main/LloydLR10KTensile/CrashReport.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -I main/LloydLR10KTensile tests/crash_report_test.cpp -o crash_report_test
//   ./crash_report_test
//
// Run automatically by .github/workflows/tests.yml.

#include "CrashReport.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace CrashReport;

static bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

static void test_exception_names()
{
    CHECK(Contains(ExceptionName(0xC0000005u), "Access violation"));
    CHECK(Contains(ExceptionName(0xC00000FDu), "Stack overflow"));
    CHECK(Contains(ExceptionName(0xE06D7363u), "C++ exception"));
    CHECK(Contains(ExceptionName(0x12345678u), "Unknown"));
}

static void test_file_stem_sorts_by_time()
{
    CHECK(FileStem(2026, 11, 7, 22, 38, 5) == "crash_2026-11-07_22-38-05");
    CHECK(FileStem(2026, 9, 30, 23, 59, 59) < FileStem(2026, 10, 1, 0, 0, 0));
}

static void test_old_reports()
{
    const std::vector<std::string> stems = { FileStem(2026, 3, 1, 0, 0, 0), FileStem(2026, 1, 1, 0, 0, 0), FileStem(2026, 2, 1, 0, 0, 0) };
    CHECK(OldReports(stems, 5).empty());
    CHECK(OldReports(stems, 3).empty());
    const std::vector<std::string> old = OldReports(stems, 1);
    CHECK(old.size() == 2);
    CHECK(old[0] == FileStem(2026, 1, 1, 0, 0, 0) && old[1] == FileStem(2026, 2, 1, 0, 0, 0)); // The newest is kept
}

static void test_format()
{
    Info info;
    info.appVersion = "2.0.0 [07.11.2026]";
    info.time = "07.11.2026 22:38:05";
    info.what = ExceptionName(0xC0000005u);
    info.code = 0xC0000005u;
    info.gotCode = true;
    info.thread = "GUI (main)";
    info.dumpFile = "crash_2026-11-07_22-38-05.dmp";
    Frame f;
    f.function = "AppMain::Draw";
    f.file = "C:\\Users\\lab\\lr10k-2027\\main\\LloydLR10KTensile\\appstructs.cpp";
    f.line = 1234;
    f.module = "app.exe";
    f.offset = 0x1a2b;
    info.stack.push_back(f);
    info.stack.push_back(Frame{}); // Unknown frame (no .pdb)
    info.lastLines = { "Connected to COM3", "Test started" };

    const std::string text = Format(info);
    CHECK(Contains(text, "Version: 2.0.0 [07.11.2026]"));
    CHECK(Contains(text, "Code:    0xC0000005"));
    CHECK(Contains(text, "#0  AppMain::Draw  (appstructs.cpp:1234)  [app.exe+0x1a2b]")); // Only the file name, not the build folder
    CHECK(Contains(text, "#1  ?  [?+0x0]"));
    CHECK(Contains(text, "  Test started\n"));
    CHECK(Contains(text, "crash_2026-11-07_22-38-05.dmp"));

    Info empty;
    empty.what = "abort() was called";
    empty.gotLastLines = false;
    const std::string text2 = Format(empty);
    CHECK(!Contains(text2, "Code:"));
    CHECK(Contains(text2, "(not available)"));
    CHECK(Contains(text2, "(could not be written)"));
    CHECK(Contains(text2, "the console was in use"));
}

static void test_recent_lines_keeps_the_last()
{
    RecentLines lines(3);
    for (int i = 0; i < 5; ++i) lines.Add("line " + std::to_string(i));
    std::vector<std::string> out;
    CHECK(lines.TryCopy(out));
    CHECK(out.size() == 3 && out.front() == "line 2" && out.back() == "line 4");
}

static void test_recent_lines_from_many_threads()
{
    // Lines are added from worker threads while the crash handler copies them (ThreadSanitizer checks this)
    RecentLines lines(50);
    std::atomic<bool> stop{ false };
    std::vector<std::thread> writers;
    for (int t = 0; t < 3; ++t)
        writers.emplace_back([&, t] { int i = 0; while (!stop) lines.Add("t" + std::to_string(t) + " " + std::to_string(i++)); });
    std::vector<std::string> out;
    bool copied = false;
    for (int i = 0; i < 20; ++i) copied |= lines.TryCopy(out, std::chrono::milliseconds(500));
    stop = true;
    for (std::thread& w : writers) w.join();
    CHECK(copied);
    CHECK(out.size() <= 50);
}

static void test_recent_lines_does_not_wait_forever()
{
    // The thread that crashed may hold the lock: TryCopy gives up instead of hanging the crash handler
    RecentLines lines;
    lines.Add("x");
    std::atomic<bool> holding{ false }, release{ false };
    std::thread holder([&] {
        auto lock = lines.Lock();
        holding = true;
        while (!release) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
    while (!holding) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::vector<std::string> out;
    const auto start = std::chrono::steady_clock::now();
    CHECK(!lines.TryCopy(out, std::chrono::milliseconds(50)));
    CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
    CHECK(out.empty());

    release = true;
    holder.join();
    CHECK(lines.TryCopy(out, std::chrono::milliseconds(50)));
    CHECK(out.size() == 1);
}

int main()
{
    test_exception_names();
    test_file_stem_sorts_by_time();
    test_old_reports();
    test_format();
    test_recent_lines_keeps_the_last();
    test_recent_lines_from_many_threads();
    test_recent_lines_does_not_wait_forever();

    if (g_failures == 0)
        std::printf("crash_report_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
