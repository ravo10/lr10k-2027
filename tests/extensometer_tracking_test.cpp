// Unit tests for ExtensometerTracking (main/LloydLR10KTensile/ExtensometerTracking.h).
//
//   g++ -std=c++17 -O1 -g -I main/LloydLR10KTensile tests/extensometer_tracking_test.cpp -o extensometer_tracking_test
//   ./extensometer_tracking_test
//
// Run automatically by .github/workflows/tests.yml.

#include "ExtensometerTracking.h"

#include <cstdio>
#include <cstdlib>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

using namespace ExtensometerTracking;

static void test_two_blobs_sorted()
{
    const Markers m = PickMarkers({ { 100, 400 }, { 102, 160 } }, Markers{});
    CHECK(m.found);
    CHECK(m.upper.y == 160 && m.lower.y == 400);
}

static void test_too_few()
{
    CHECK(!PickMarkers({}, Markers{}).found);
    CHECK(!PickMarkers({ { 1, 2 } }, Markers{}).found);
}

static void test_extra_blob_is_ignored()
{
    Markers previous;
    previous.found = true;
    previous.upper = { 100, 160 };
    previous.lower = { 100, 400 };

    // A reflection at (300, 250): the markers moved a little and are still used
    const Markers m = PickMarkers({ { 300, 250 }, { 101, 162 }, { 99, 405 } }, previous);
    CHECK(m.found);
    CHECK(m.upper.x == 101 && m.upper.y == 162);
    CHECK(m.lower.x == 99 && m.lower.y == 405);

    // Without a previous position the extra blob cannot be told apart
    CHECK(!PickMarkers({ { 300, 250 }, { 101, 162 }, { 99, 405 } }, Markers{}).found);

    // A marker moved too far since the last good frame: not followed
    CHECK(!PickMarkers({ { 300, 250 }, { 101, 162 }, { 99, 480 } }, previous).found);
}

static void test_same_blob_is_not_used_twice()
{
    Markers previous;
    previous.found = true;
    previous.upper = { 100, 200 };
    previous.lower = { 100, 210 }; // close together (unusual, but must not pick one blob twice)
    const Markers m = PickMarkers({ { 100, 205 }, { 400, 50 }, { 400, 60 } }, previous);
    CHECK(!m.found);
}

static void test_jump_gate()
{
    JumpGate gate(0.02, 4); // 2 % of L0 per frame, believed after 4 frames
    const double l0 = 240.0; // limit 4.8 px
    CHECK(gate.Accept(240.0, 240.0, l0));
    CHECK(gate.Accept(244.0, 240.0, l0));   // 4 px: normal movement
    CHECK(!gate.Accept(260.0, 240.0, l0));  // 20 px in one frame: a bad frame
    CHECK(gate.Accept(240.5, 240.0, l0));   // back to normal: the bad frame was ignored
    CHECK(gate.RejectedInARow() == 0);

    // A real fast change (fracture): believed once it stays for 4 frames
    CHECK(!gate.Accept(300.0, 240.0, l0));
    CHECK(!gate.Accept(300.0, 240.0, l0));
    CHECK(!gate.Accept(300.0, 240.0, l0));
    CHECK(gate.Accept(300.0, 240.0, l0));
}

int main()
{
    test_two_blobs_sorted();
    test_too_few();
    test_extra_blob_is_ignored();
    test_same_blob_is_not_used_twice();
    test_jump_gate();

    if (g_failures == 0)
        std::printf("extensometer_tracking_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
