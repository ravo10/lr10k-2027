// Unit tests for LoadSafety (main/LloydLR10KTensile/LoadSafety.h): the 50 N blocker and the load limit.
//
//   g++ -std=c++17 -O1 -g -I main/LloydLR10KTensile tests/load_safety_test.cpp -o load_safety_test
//   ./load_safety_test
//
// Run automatically by .github/workflows/tests.yml.

#include "LoadSafety.h"

#include <cstdio>
#include <cstdlib>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using LoadSafety::Guard;

static void test_effective_limit()
{
    CHECK(LoadSafety::EffectiveLimit(500.0) == 500.0);
    CHECK(LoadSafety::EffectiveLimit(20000.0) == 10000.0); // never above the machine
    CHECK(LoadSafety::EffectiveLimit(0.0) == 10000.0);
    CHECK(LoadSafety::EffectiveLimit(-5.0) == 10000.0);
    CHECK(LoadSafety::OverLimit(500.0, 500.0));
    CHECK(LoadSafety::OverLimit(-600.0, 500.0)); // compression
    CHECK(!LoadSafety::OverLimit(499.9, 500.0));
}

static void test_moving_down_onto_something()
{
    Guard g; // 50 N
    long long t = 0;
    CHECK(!g.Update(true, 0.0, t += 50));
    CHECK(!g.Update(true, 20.0, t += 50));
    CHECK(!g.Update(true, 49.0, t += 50));
    CHECK(g.Update(true, 51.0, t += 50));  // above 50 N and rising: stop
    CHECK(!g.Update(true, 51.5, t += 50)); // not 2 N more since the stop
    CHECK(!g.Update(true, 54.0, t += 50)); // 2 N more, but within 1 s of the last stop
    CHECK(g.Update(true, 56.0, 2000));     // still rising after 1 s: stop again
}

static void test_compression_sign()
{
    Guard g;
    CHECK(!g.Update(true, -10.0, 0));
    CHECK(g.Update(true, -60.0, 50));
}

static void test_moving_away_is_allowed()
{
    // A clamped specimen holds 80 N; the user moves the crosshead to relieve it
    Guard g;
    long long t = 0;
    for (double load = 80.0; load > 0.0; load -= 3.0)
        CHECK(!g.Update(true, load, t += 50));
    // ... and back towards it again: stop once it rises 2 N above the lowest load and is over 50 N
    CHECK(!g.Update(true, 30.0, t += 50));
    CHECK(g.Update(true, 52.0, t += 50));
}

static void test_noise_while_stationary_above_the_limit()
{
    // Above 50 N, not moving, load noise of one count (0.76 N): no stop
    Guard g;
    long long t = 0;
    for (int i = 0; i < 100; ++i)
        CHECK(!g.Update(true, 70.0 + (i % 2) * 0.76, t += 50));
}

static void test_inactive_resets()
{
    Guard g;
    CHECK(!g.Update(true, 10.0, 0));
    CHECK(!g.Update(false, 60.0, 50)); // not moving / test running / blocker off: nothing
    // Next movement starts at 60 N (lowest = 60): moving away does nothing, moving in stops
    CHECK(!g.Update(true, 60.0, 100));
    CHECK(!g.Update(true, 58.0, 150));
    CHECK(g.Update(true, 61.0, 200));
}

static void test_custom_limit()
{
    Guard g(500.0);
    CHECK(!g.Update(true, 0.0, 0));
    CHECK(!g.Update(true, 499.0, 50));
    CHECK(g.Update(true, 501.0, 100));
}

int main()
{
    test_effective_limit();
    test_moving_down_onto_something();
    test_compression_sign();
    test_moving_away_is_allowed();
    test_noise_while_stationary_above_the_limit();
    test_inactive_resets();
    test_custom_limit();

    if (g_failures == 0)
        std::printf("load_safety_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
