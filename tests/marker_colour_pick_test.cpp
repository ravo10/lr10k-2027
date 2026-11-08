// Unit tests for MarkerColourPick (main/LloydLR10KTensile/MarkerColourPick.h): the eyedropper's HSV ranges.
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/marker_colour_pick_test.cpp -o marker_colour_pick_test
//   ./marker_colour_pick_test
//
// Run automatically by .github/workflows/tests.yml.

#include "MarkerColourPick.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace MarkerColourPick;

static std::vector<unsigned char> Pixels(int n, int h, int s, int v)
{
    std::vector<unsigned char> p;
    for (int i = 0; i < n; ++i) { p.push_back((unsigned char)h); p.push_back((unsigned char)s); p.push_back((unsigned char)v); }
    return p;
}

static void test_green_marker()
{
    // #00FF00 under lab light: OpenCV hue 60, a bit less than full saturation and brightness
    const auto p = Pixels(25, 60, 200, 190);
    const Result r = FromPixels(p.data(), 25);
    CHECK(r.ok);
    CHECK(r.h == 60 && r.s == 200 && r.v == 190);
    CHECK(r.range.hMin == 45 && r.range.hMax == 75);
    CHECK(r.range.sMin == 120 && r.range.sMax == 255);
    CHECK(r.range.vMin == 100 && r.range.vMax == 255);
    // The sample itself is inside the range, and a shaded edge of the marker too
    CHECK(60 >= r.range.hMin && 60 <= r.range.hMax && 200 >= r.range.sMin && 190 >= r.range.vMin);
    CHECK(150 >= r.range.sMin && 120 >= r.range.vMin);
}

static void test_median_ignores_some_background()
{
    // 25 pixels: 18 marker, 7 of the white specimen next to it (the click was a bit off the middle)
    auto p = Pixels(18, 62, 210, 200);
    const auto white = Pixels(7, 20, 10, 240);
    p.insert(p.end(), white.begin(), white.end());
    const Result r = FromPixels(p.data(), 25);
    CHECK(r.ok);
    CHECK(r.h == 62 && r.s == 210 && r.v == 200);
}

static void test_not_a_marker()
{
    const auto white = Pixels(25, 30, 15, 245);  // the specimen or paper
    const auto grey = Pixels(25, 100, 40, 120);  // a grip
    const auto black = Pixels(25, 60, 200, 25);  // a shadow
    CHECK(!FromPixels(white.data(), 25).ok);
    CHECK(!FromPixels(grey.data(), 25).ok);
    CHECK(!FromPixels(black.data(), 25).ok);
    CHECK(!FromPixels(nullptr, 0).ok);
}

static void test_limits()
{
    // Hue at the ends of OpenCV's range, a weak but acceptable colour: clamped, never below the minimums
    const auto red = Pixels(9, 3, 90, 60);
    const Result r = FromPixels(red.data(), 9);
    CHECK(r.ok);
    CHECK(r.range.hMin == 0 && r.range.hMax == 18);
    CHECK(r.range.sMin == kMinSat && r.range.vMin == kMinVal);
    const auto magenta = Pixels(9, 175, 255, 255);
    const Result m = FromPixels(magenta.data(), 9);
    CHECK(m.range.hMax == 179 && m.range.hMin == 160);
}

static void test_sample_radius()
{
    CHECK(SampleRadius(640) == 2);
    CHECK(SampleRadius(800) == 2);   // 5 x 5 pixels
    CHECK(SampleRadius(1920) == 4);
    CHECK(SampleRadius(3840) == 8);  // 17 x 17 pixels (a Ø5 mm marker is ~40 px wide at 4K, L0 60 mm)
}

int main()
{
    test_green_marker();
    test_median_ignores_some_background();
    test_not_a_marker();
    test_limits();
    test_sample_radius();
    if (g_failures == 0)
        std::printf("marker_colour_pick_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
