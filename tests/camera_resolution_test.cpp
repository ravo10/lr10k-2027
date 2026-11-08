// Unit tests for CameraResolution (main/LloydLR10KTensile/CameraResolution.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/camera_resolution_test.cpp -o camera_resolution_test
//   ./camera_resolution_test
//
// Run automatically by .github/workflows/tests.yml.

#include "CameraResolution.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace CameraResolution;

static void test_options()
{
    CHECK(kOptions[0].width == 800 && kOptions[0].height == 600); // The old fixed size is the default
    CHECK(kOptions[kOptionCount - 1].width == 3840);
    for (int i = 1; i < kOptionCount; ++i)
        CHECK(kOptions[i].width > kOptions[i - 1].width);
    CHECK(ClampIndex(-3) == 0 && ClampIndex(99) == kOptionCount - 1 && ClampIndex(2) == 2);
}

static void test_work_scale()
{
    CHECK(WorkScale(800) == 1.0);
    CHECK(WorkScale(1280) == 1.0);
    CHECK(std::fabs(WorkScale(1920) - 1.5) < 1e-12);
    CHECK(std::fabs(WorkScale(3840) - 3.0) < 1e-12);
}

static void test_raw_recording_size()
{
    int w = 0, h = 0;
    RawRecordingSize(1280, 720, w, h);
    CHECK(w == 1280 && h == 720);
    RawRecordingSize(1920, 1080, w, h);
    CHECK(w == 1920 && h == 1080);
    RawRecordingSize(3840, 2160, w, h);
    CHECK(w == 1920 && h == 1080);
    RawRecordingSize(2560, 1440, w, h);
    CHECK(w == 1920 && h == 1080);
    RawRecordingSize(3000, 2001, w, h); // An odd size: the height stays even
    CHECK(w == 1920 && h % 2 == 0);
}

static void test_marker_move()
{
    CHECK(MaxMarkerMovePx(800) == 25.0);
    CHECK(MaxMarkerMovePx(640) == 25.0); // Not less than before
    CHECK(std::fabs(MaxMarkerMovePx(3840) - 120.0) < 1e-12);
}

static void test_rect_and_positions()
{
    // Scale 1: only the padding
    Rect r = ToFullResolution(Rect{ 10, 20, 5, 6 }, 1.0, 3, 800, 600);
    CHECK(r.x == 7 && r.y == 17 && r.width == 11 && r.height == 12);

    // Scale 3 (4K): working pixels 10..14 are full pixels 30..44, plus padding
    r = ToFullResolution(Rect{ 10, 20, 5, 6 }, 3.0, 3, 3840, 2160);
    CHECK(r.x == 27 && r.y == 57 && r.width == 15 + 6 && r.height == 18 + 6);

    // Clipped at the frame's edges
    r = ToFullResolution(Rect{ 0, 0, 5, 5 }, 3.0, 3, 3840, 2160);
    CHECK(r.x == 0 && r.y == 0 && r.width == 18 && r.height == 18);
    r = ToFullResolution(Rect{ 1275, 715, 5, 5 }, 3.0, 3, 3840, 2160);
    CHECK(r.x + r.width == 3840 && r.y + r.height == 2160);

    // A position there and back; the centre of working pixel 0 is the centre of full pixels 0..2 (= 1)
    CHECK(std::fabs(WorkToFull(0.0, 3.0) - 1.0) < 1e-12);
    CHECK(std::fabs(FullToWork(WorkToFull(123.4, 1.5), 1.5) - 123.4) < 1e-9);
    CHECK(WorkToFull(57.25, 1.0) == 57.25);
}

int main()
{
    test_options();
    test_work_scale();
    test_raw_recording_size();
    test_marker_move();
    test_rect_and_positions();

    if (g_failures == 0)
        std::printf("camera_resolution_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
