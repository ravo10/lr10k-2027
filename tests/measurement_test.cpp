// Unit tests for LR10KMeasurement (main/LloydLR10KTensile/LR10KMeasurement.h).
//
//   g++ -std=c++17 -O1 -g -pthread -I main/LloydLR10KTensile tests/measurement_test.cpp -o measurement_test
//   ./measurement_test
//
// Run automatically by .github/workflows/tests.yml.

#include "LR10KMeasurement.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define NEAR(a, b) CHECK(std::fabs((a) - (b)) < 1e-9)

// The app's calibration: 0.76294 N and 1/15556 mm per count
static const LR10KMeasurement::Calibration kCalibration{ 0.76294, 1.0 / 15556.0 };

static LR10KReadings readings(std::int16_t load, std::int64_t distance, std::uint64_t packages)
{
    LR10KReadings r;
    r.load = load;
    r.distance = distance;
    r.dataPackages = packages;
    return r;
}

static void test_first_package_is_the_zero()
{
    LR10KMeasurement m(kCalibration);

    // Connected, no data yet: load 0, waiting for the zero
    auto s = m.FromReadings(readings(0, 0, 0));
    CHECK(s.loadZeroPending);
    NEAR(s.loadNewton, 0.0);

    // First package (the idle reading of the recorded session: -49 counts) becomes the zero
    s = m.FromReadings(readings(-49, 0, 1));
    CHECK(!s.loadZeroPending);
    NEAR(s.loadNewton, 0.0);

    // 100 counts more load = 76.294 N
    s = m.FromReadings(readings(51, 0, 2));
    NEAR(s.loadNewton, 100 * 0.76294);
}

static void test_zero_request_uses_the_next_package()
{
    LR10KMeasurement m(kCalibration);
    m.FromReadings(readings(10, 0, 1)); // zero = 10
    NEAR(m.FromReadings(readings(30, 0, 2)).loadNewton, 20 * 0.76294);

    // "Set zero point" while the load reads 30: load is 0 until the next package, which becomes the zero
    m.RequestLoadZero(readings(30, 0, 2));
    auto s = m.FromReadings(readings(30, 0, 2)); // same package again (GUI frame before new data)
    CHECK(s.loadZeroPending);
    NEAR(s.loadNewton, 0.0);
    s = m.FromReadings(readings(31, 0, 3));      // next package: zero = 31
    CHECK(!s.loadZeroPending);
    NEAR(s.loadNewton, 0.0);
    NEAR(m.FromReadings(readings(41, 0, 4)).loadNewton, 10 * 0.76294);
}

static void test_zero_reading_of_exactly_zero_counts()
{
    // The old "offset 0 = take the next reading" rule re-zeroed on every reading when the zero reading was 0
    LR10KMeasurement m(kCalibration);
    m.FromReadings(readings(0, 0, 1)); // zero = 0 counts
    NEAR(m.FromReadings(readings(100, 0, 2)).loadNewton, 100 * 0.76294);
    NEAR(m.FromReadings(readings(200, 0, 3)).loadNewton, 200 * 0.76294);
}

static void test_extension_and_reset()
{
    LR10KMeasurement m(kCalibration);
    // 15556 counts = 1 mm; 4.7 mm = 73113.2 counts
    NEAR(m.FromReadings(readings(0, 15556, 1)).extensionMm, 1.0);
    NEAR(m.FromReadings(readings(0, 73113, 2)).extensionMm, 73113.0 / 15556.0);
    NEAR(m.FromReadings(readings(0, -15556, 3)).extensionMm, -1.0);

    // Disconnect: zero is forgotten, the first package after reconnecting is the new zero
    m.Reset();
    auto s = m.FromReadings(readings(500, 0, 0)); // the machine's package count restarts too
    CHECK(s.loadZeroPending);
    s = m.FromReadings(readings(500, 0, 1));
    NEAR(s.loadNewton, 0.0);
    NEAR(m.FromReadings(readings(510, 0, 2)).loadNewton, 10 * 0.76294);
}

static void test_with_the_machine()
{
    // Together with LR10KMachine::Apply / ResetDistance, like in the app
    LR10KMachine machine;
    LR10KMeasurement m(kCalibration);
    LR10KProtocol::Message pkg;
    pkg.type = LR10KProtocol::Message::Type::DataPackage;

    pkg.data = { 0xEF, 0x4F, 0x00, 0x00, 0x00, 0x00 };     // idle: -49 counts, no movement
    machine.Apply(pkg);
    NEAR(m.FromReadings(machine.Readings()).loadNewton, 0.0); // first package = zero

    pkg.data = { 0xF0, 0x30, 0x01, 0x00, 0x00, 0x00 };     // load +48 counts (97 above zero), distance +512
    machine.Apply(pkg);
    auto s = m.FromReadings(machine.Readings());
    NEAR(s.loadNewton, 97 * 0.76294);
    NEAR(s.extensionMm, 512.0 / 15556.0);

    machine.ResetDistance();                                // zero point for the extension
    NEAR(m.FromReadings(machine.Readings()).extensionMm, 0.0);
}

int main()
{
    test_first_package_is_the_zero();
    test_zero_request_uses_the_next_package();
    test_zero_reading_of_exactly_zero_counts();
    test_extension_and_reset();
    test_with_the_machine();

    if (g_failures == 0)
        std::printf("measurement_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
