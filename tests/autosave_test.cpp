// Unit tests for Autosave (main/LloydLR10KTensile/Autosave.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -I main/LloydLR10KTensile tests/autosave_test.cpp -o autosave_test
//   ./autosave_test
//
// Run automatically by .github/workflows/tests.yml.

#include "Autosave.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace Autosave;
namespace fs = std::filesystem;

static bool Has(const std::vector<int>& ids, int id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); }

static std::string ReadAll(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

static void test_file_names()
{
    const std::string session = SessionStamp(2026, 11, 8, 10, 15, 2, 4242);
    CHECK(session == "20261108-101502-4242");
    const std::string name = FileName(session, 3);
    CHECK(name == "autosave_20261108-101502-4242_tab3.lr10k");

    std::string parsedSession;
    int tabId = -1;
    CHECK(ParseFileName(name, parsedSession, tabId) && parsedSession == session && tabId == 3);
    CHECK(!ParseFileName("my test.lr10k", parsedSession, tabId));
    CHECK(!ParseFileName("autosave_x_tab.lr10k", parsedSession, tabId));
    CHECK(!ParseFileName("autosave_x_tab3.lr10k.tmp", parsedSession, tabId)); // A half-written file is not recovered
    CHECK(!ParseFileName("autosave__tab3.lr10k", parsedSession, tabId));
    CHECK(!ParseFileName("autosave_x_tab1a.lr10k", parsedSession, tabId));

    const std::vector<std::string> names = { FileName("old", 1), FileName("old", 2), FileName(session, 1), "other.lr10k" };
    const std::vector<std::string> recover = FilesToRecover(names, session);
    CHECK(recover.size() == 2 && recover[0] == FileName("old", 1));
}

static void test_change_key()
{
    CHECK(ChangeKey().Add(1).Value() == ChangeKey().Add(1).Value());
    CHECK(ChangeKey().Add(1).Value() != ChangeKey().Add(2).Value());
    CHECK(ChangeKey().Add(std::string("ab")).Add(std::string("c")).Value() != ChangeKey().Add(std::string("a")).Add(std::string("bc")).Value());
}

static void test_scheduler()
{
    Scheduler s;
    TabState running{ 1, true, true, 100 };
    TabState saved{ 2, false, false, 0 };

    // First time: written at once
    Plan p = s.Update({ running, saved }, 0);
    CHECK(Has(p.save, 1) && !Has(p.save, 2) && p.remove.empty());

    // Changed, but the interval has not passed
    running.changeKey = 101;
    p = s.Update({ running, saved }, 1000);
    CHECK(p.save.empty());

    // Interval passed: written
    p = s.Update({ running, saved }, kIntervalDuringTestMs);
    CHECK(Has(p.save, 1));

    // Interval passed, but nothing changed: not written again
    p = s.Update({ running, saved }, 3 * kIntervalDuringTestMs);
    CHECK(p.save.empty());

    // Not running any more: the longer interval
    running.testRunning = false;
    running.changeKey = 102;
    const long long lastWrite = kIntervalDuringTestMs; // The last write above
    p = s.Update({ running, saved }, lastWrite + kIntervalDuringTestMs);
    CHECK(p.save.empty());
    p = s.Update({ running, saved }, lastWrite + kIntervalOtherwiseMs);
    CHECK(Has(p.save, 1));

    // Saved by the user: its file goes
    running.needsAutosave = false;
    p = s.Update({ running, saved }, 100000);
    CHECK(Has(p.remove, 1) && p.save.empty());
    p = s.Update({ running, saved }, 100001);
    CHECK(p.remove.empty()); // Only once

    // A tab with a file is closed: its file goes
    TabState other{ 3, true, false, 7 };
    p = s.Update({ other }, 200000);
    CHECK(Has(p.save, 3));
    CHECK(s.TabsWithFiles().size() == 1);
    p = s.Update({}, 200001);
    CHECK(Has(p.remove, 3));
    CHECK(s.TabsWithFiles().empty());
}

static void test_atomic_write()
{
    const fs::path folder = fs::temp_directory_path() / "lr10k_autosave_test";
    fs::remove_all(folder);
    fs::create_directories(folder);
    const fs::path file = folder / "a.lr10k";

    CHECK(WriteFileAtomically(file, "first"));
    CHECK(ReadAll(file) == "first");
    CHECK(WriteFileAtomically(file, "second, longer"));
    CHECK(ReadAll(file) == "second, longer");
    CHECK(!fs::exists(folder / "a.lr10k.tmp"));
    CHECK(!WriteFileAtomically(folder / "missing" / "b.lr10k", "x"));
    fs::remove_all(folder);
}

static void test_writer()
{
    const fs::path folder = fs::temp_directory_path() / "lr10k_autosave_writer_test";
    fs::remove_all(folder);
    fs::create_directories(folder);

    std::atomic<int> reports{ 0 }, produced{ 0 };
    {
        Writer writer([&](const fs::path&, bool ok) { if (ok) ++reports; });
        // Many writes to the same file from the GUI thread: the newest wins, older waiting ones are skipped
        for (int i = 0; i < 200; ++i)
            writer.Write(folder / "tab1.lr10k", [i, &produced] { ++produced; return "version " + std::to_string(i); });
        writer.Write(folder / "tab2.lr10k", [] { return std::string("two"); });
        writer.Flush();
        CHECK(ReadAll(folder / "tab1.lr10k") == "version 199");
        CHECK(ReadAll(folder / "tab2.lr10k") == "two");
        CHECK(produced >= 1 && produced <= 200);

        // A delete after a write that has not run yet: the file ends up gone
        writer.Write(folder / "tab3.lr10k", [] { return std::string("three"); });
        writer.Remove(folder / "tab3.lr10k");
        writer.Remove(folder / "tab2.lr10k");
        writer.Flush();
        CHECK(!fs::exists(folder / "tab3.lr10k"));
        CHECK(!fs::exists(folder / "tab2.lr10k"));

        // A producer that throws does not stop the writer
        writer.Write(folder / "bad.lr10k", []() -> std::string { throw std::runtime_error("no"); });
        writer.Write(folder / "good.lr10k", [] { return std::string("ok"); });
        writer.Flush();
        CHECK(ReadAll(folder / "good.lr10k") == "ok");

        // Work handed over just before the end is still done
        writer.Write(folder / "last.lr10k", [] { return std::string("last"); });
    } // Destructor: Stop()
    CHECK(ReadAll(folder / "last.lr10k") == "last");
    CHECK(reports >= 4);
    fs::remove_all(folder);
}

int main()
{
    test_file_names();
    test_change_key();
    test_scheduler();
    test_atomic_write();
    test_writer();

    if (g_failures == 0)
        std::printf("autosave_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
