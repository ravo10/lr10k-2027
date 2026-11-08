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
    // A weak but acceptable colour: saturation and brightness never below the minimums
    const auto weak = Pixels(9, 30, 90, 60);
    const Result r = FromPixels(weak.data(), 9);
    CHECK(r.ok);
    CHECK(r.range.sMin == kMinSat && r.range.vMin == kMinVal);
}

static void test_red_wraps_around()
{
    // A red marker near hue 0: the range goes through 179 and 0 instead of being cut at 0
    const auto red = Pixels(9, 3, 220, 200);
    const Result r = FromPixels(red.data(), 9);
    CHECK(r.ok);
    CHECK(r.range.hMin == 168 && r.range.hMax == 18);
    CHECK(HueWraps(r.range.hMin, r.range.hMax));
    CHECK(InHueRange(175, r.range.hMin, r.range.hMax) && InHueRange(0, r.range.hMin, r.range.hMax) && InHueRange(10, r.range.hMin, r.range.hMax));
    CHECK(!InHueRange(90, r.range.hMin, r.range.hMax) && !InHueRange(30, r.range.hMin, r.range.hMax));
    // Magenta-red near 179
    const auto magenta = Pixels(9, 175, 255, 255);
    const Result m = FromPixels(magenta.data(), 9);
    CHECK(m.range.hMin == 160 && m.range.hMax == 10);

    // A red marker whose pixels are on both sides of 0/179: the median is red (not ~90, cyan)
    std::vector<unsigned char> mixed;
    for (int h : { 177, 178, 179, 179, 0, 0, 1, 1, 2 }) { mixed.push_back((unsigned char)h); mixed.push_back(210); mixed.push_back(200); }
    const Result x = FromPixels(mixed.data(), 9);
    CHECK(x.ok);
    CHECK(x.h == 179 || x.h == 0);
    CHECK(InHueRange(177, x.range.hMin, x.range.hMax) && InHueRange(2, x.range.hMin, x.range.hMax));
}

static void test_hue_helpers()
{
    // Plain range
    CHECK(HueWidth(25, 85) == 60);
    CHECK(HueCentre(25, 85) == 55.0);
    // Wrapping range 168..12: 24 steps, centre 0
    CHECK(HueWidth(168, 12) == 24);
    CHECK(HueCentre(168, 12) == 0.0);
    CHECK(HueCentre(170, 10) == 0.0);
    CHECK(HueCentre(160, 10) == 175.0);
    // Around a centre, both ways round
    HsvRange a = HueRangeAround(5, 15);
    CHECK(a.hMin == 170 && a.hMax == 20);
    a = HueRangeAround(60, 15);
    CHECK(a.hMin == 45 && a.hMax == 75);
    a = HueRangeAround(90, 100); // wider than the circle: everything
    CHECK(a.hMin == 0 && a.hMax == 179);
    // The parts for cv::inRange
    HueParts p = MaskHueParts(25, 85);
    CHECK(p.count == 1 && p.lo[0] == 25 && p.hi[0] == 85);
    p = MaskHueParts(168, 12);
    CHECK(p.count == 2 && p.lo[0] == 168 && p.hi[0] == 179 && p.lo[1] == 0 && p.hi[1] == 12);
    // Every hue is in exactly the parts InHueRange says
    for (int lo = 0; lo < 180; lo += 7)
        for (int hi = 0; hi < 180; hi += 11)
        {
            const HueParts q = MaskHueParts(lo, hi);
            for (int h = 0; h < 180; ++h)
            {
                bool inParts = false;
                for (int i = 0; i < q.count; ++i) inParts |= (h >= q.lo[i] && h <= q.hi[i]);
                CHECK(inParts == InHueRange(h, lo, hi));
            }
        }
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
    test_red_wraps_around();
    test_hue_helpers();
    test_sample_radius();
    if (g_failures == 0)
        std::printf("marker_colour_pick_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
