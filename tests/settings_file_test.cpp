// Unit tests for SettingsFile (main/LloydLR10KTensile/SettingsFile.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/settings_file_test.cpp -o settings_file_test
//   ./settings_file_test
//
// Run automatically by .github/workflows/tests.yml.

#include "SettingsFile.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

namespace fs = std::filesystem;
using namespace SettingsFile;

static void Write(const fs::path& path, const std::string& text, int minutesAgo)
{
    std::ofstream(path) << text;
    fs::last_write_time(path, fs::file_time_type::clock::now() - std::chrono::minutes(minutesAgo));
}

static std::string Read(const fs::path& path)
{
    std::ifstream in(path);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

static void test_names()
{
    CHECK(IsOldName("v1.1.2 [26.02.2026]_lr10k.ini"));
    CHECK(IsOldName("v1.0.3_lr10k.ini"));
    CHECK(!IsOldName("lr10k_settings.ini"));
    CHECK(!IsOldName("imgui.ini"));
    CHECK(!IsOldName("v_lr10k.ini.bak"));
}

static void test_move_old_settings()
{
    const fs::path folder = fs::temp_directory_path() / "lr10k_settings_file_test";
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::path from;

    // Nothing there: nothing copied (the app writes default settings)
    CHECK(!MoveOldSettings(folder, from));

    // Two old files: the newest is copied, both stay
    Write(folder / "v1.0.3_lr10k.ini", "{\"old\":1}", 600);
    Write(folder / "v1.1.2 [26.02.2026]_lr10k.ini", "{\"old\":2}", 60);
    Write(folder / "imgui.ini", "[Window]", 1);
    CHECK(MoveOldSettings(folder, from));
    CHECK(from.filename() == "v1.1.2 [26.02.2026]_lr10k.ini");
    CHECK(Read(folder / kFileName) == "{\"old\":2}");
    CHECK(fs::exists(folder / "v1.0.3_lr10k.ini") && fs::exists(folder / "v1.1.2 [26.02.2026]_lr10k.ini"));

    // The new file exists: never overwritten again
    Write(folder / "v9.9.9_lr10k.ini", "{\"old\":3}", 0);
    std::ofstream(folder / kFileName) << "{\"new\":1}";
    CHECK(!MoveOldSettings(folder, from));
    CHECK(Read(folder / kFileName) == "{\"new\":1}");

    // A folder that does not exist: nothing happens
    CHECK(!MoveOldSettings(folder / "missing", from));
    fs::remove_all(folder);
}

int main()
{
    test_names();
    test_move_old_settings();

    if (g_failures == 0)
        std::printf("settings_file_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
