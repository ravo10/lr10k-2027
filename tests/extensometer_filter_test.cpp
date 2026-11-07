// Unit tests for ExtensometerFilter (main/LloydLR10KTensile/ExtensometerFilter.h).
//
//   g++ -std=c++17 -O1 -g -I main/LloydLR10KTensile tests/extensometer_filter_test.cpp -o extensometer_filter_test
//   ./extensometer_filter_test
//
// Run automatically by .github/workflows/tests.yml.

#include "ExtensometerFilter.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define NEAR(a, b, tol) CHECK(std::fabs((a) - (b)) <= (tol))

static void test_first_value_is_l0()
{
    ExtensometerFilter f;
    CHECK(!f.Initialized());
    f.Update(400.0, 0.04);
    CHECK(f.Initialized());
    NEAR(f.InitialPx(), 400.0, 1e-12);
    NEAR(f.Strain(), 0.0, 1e-12);
}

static void test_step_settles_with_the_time_constant()
{
    // A step of 4 px (1 % of 400 px): after one time constant 63 %, after five ~99 %
    ExtensometerFilter f(0.25);
    f.Reset(400.0);
    for (int i = 0; i < 6; ++i) f.Update(404.0, 0.04);
    // 6 steps of 0.04 s = 0.24 s: 1 - e^(-0.24/0.25) = 61.7 %
    NEAR(f.SmoothPx() - 400.0, 4.0 * (1.0 - std::exp(-0.24 / 0.25)), 1e-9);
    for (int i = 0; i < 25; ++i) f.Update(404.0, 0.04);
    NEAR(f.Strain(), 0.01, 0.15 / 400.0 + 1e-9); // stops within the 0.15 px dead-band
}

static void test_same_lag_at_any_frame_rate()
{
    // 15 and 60 frames/s give the same value after the same time (the old per-frame weight did not)
    ExtensometerFilter slow(0.25), fast(0.25);
    slow.Reset(400.0);
    fast.Reset(400.0);
    for (int i = 0; i < 15; ++i) slow.Update(410.0, 1.0 / 15.0);
    for (int i = 0; i < 60; ++i) fast.Update(410.0, 1.0 / 60.0);
    NEAR(slow.SmoothPx(), fast.SmoothPx(), 1e-9);
}

static void test_ramp_lag_is_small()
{
    // A ramp (constant crosshead speed): the filtered value trails by about speed * tau
    ExtensometerFilter f(0.25);
    f.Reset(400.0);
    const double pxPerS = 2.0, dt = 0.04;
    double raw = 400.0;
    for (int i = 0; i < 200; ++i) { raw += pxPerS * dt; f.Update(raw, dt); }
    const double lagS = (raw - f.SmoothPx()) / pxPerS;
    CHECK(lagS > 0.2 && lagS < 0.3); // ~0.25 s (the old filter: ~1.3 s)
}

static void test_deadband()
{
    ExtensometerFilter f(0.25, 0.15);
    f.Reset(400.0);
    for (int i = 0; i < 100; ++i) f.Update(400.0 + ((i % 2) ? 0.1 : -0.1), 0.04); // pixel noise
    NEAR(f.SmoothPx(), 400.0, 1e-12);
}

static void test_no_smoothing_and_reset()
{
    ExtensometerFilter f(0.0);
    f.Reset(400.0);
    f.Update(420.0, 0.04);
    NEAR(f.Strain(), 0.05, 1e-12);
    f.Reset(420.0);
    NEAR(f.Strain(), 0.0, 1e-12);
}

int main()
{
    test_first_value_is_l0();
    test_step_settles_with_the_time_constant();
    test_same_lag_at_any_frame_rate();
    test_ramp_lag_is_small();
    test_deadband();
    test_no_smoothing_and_reset();

    if (g_failures == 0)
        std::printf("extensometer_filter_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
