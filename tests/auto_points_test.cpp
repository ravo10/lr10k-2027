// Unit tests for AutoPoints (main/LloydLR10KTensile/AutoPoints.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I main/LloydLR10KTensile tests/auto_points_test.cpp -o auto_points_test
//   ./auto_points_test
//
// Run automatically by .github/workflows/tests.yml.
//
// The curves are made with the Ramberg-Osgood formula, strain = stress / E + 0.002 * (stress / Rp0.2)^n:
// its 0.2 % offset yield is exactly Rp0.2, and its 0.01 % offset yield is Rp0.2 * (0.0001 / 0.002)^(1/n).

#include "AutoPoints.h"
#include "TestData.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define NEAR_REL(a, b, rel) do { const double a_ = (a), b_ = (b); if (!(std::fabs(a_ - b_) <= (rel) * std::fabs(b_))) { \
    std::printf("FAIL line %d: %s = %g, expected %g (+- %g %%)\n", __LINE__, #a, a_, b_, (rel) * 100); ++g_failures; } } while (0)

using namespace AutoPoints;

struct Vec2 { float x = 0, y = 0; };  // Like ImVec2
using Curve = std::vector<Vec2>;

constexpr double kL0 = 50.0;   // mm
constexpr double kA0 = 20.0;   // mm^2

struct Material
{
    double E;      // N/mm^2
    double rp02;   // N/mm^2
    double n;      // Ramberg-Osgood exponent
    double uts;    // N/mm^2 (the curve rises to here, then falls)
};

/**
    A test: load (N) against crosshead extension (mm).
    - slackMm: at the start the load rises slowly (slack, grips settling) over this extension;
    - noiseN: load noise (then smoothed like the app does, with the default window 5).
**/
static Curve MakeTest(const Material& m, double slackMm, double noiseN, int points = 2000, unsigned seed = 1)
{
    Curve c;
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0.0, noiseN > 0 ? noiseN : 1e-12);

    // Slack: load grows with the square of the extension, joining the elastic line smoothly at 5 % of the UTS load
    const double joinLoad = 0.05 * m.uts * kA0;
    const int slackPoints = slackMm > 0 ? points / 10 : 0;
    for (int i = 0; i < slackPoints; ++i)
    {
        const double t = (double)i / slackPoints;
        c.push_back({ (float)(t * slackMm), (float)(joinLoad * t * t + noise(rng)) });
    }
    // Ramberg-Osgood from the join load up to the UTS, then a drop (necking)
    const double startStress = slackPoints > 0 ? joinLoad / kA0 : 0.0;
    const double startStrain = startStress / m.E + 0.002 * std::pow(startStress / m.rp02, m.n);
    const int rising = points - slackPoints - points / 10;
    double lastX = slackMm;
    for (int i = 1; i <= rising; ++i)
    {
        const double stress = startStress + (m.uts - startStress) * (double)i / rising;
        const double strain = stress / m.E + 0.002 * std::pow(stress / m.rp02, m.n) - startStrain;
        lastX = slackMm + strain * kL0;
        c.push_back({ (float)lastX, (float)(stress * kA0 + noise(rng)) });
    }
    for (int i = 1; i <= points / 10; ++i)
        c.push_back({ (float)(lastX + 0.01 * i), (float)(m.uts * kA0 * (1.0 - 0.003 * i) + noise(rng)) });

    return noiseN > 0 ? TestData::MovingAverageY(c, 5) : c;
}

static double Rp001(const Material& m) { return m.rp02 * std::pow(0.0001 / 0.002, 1.0 / m.n); }

static void test_clean_aluminium()
{
    const Material al{ 70000, 250, 12, 310 };
    const Curve c = MakeTest(al, 0.0, 0.0);
    const Placement p = Place(c, kL0);
    CHECK(p.elastic.ok);
    NEAR_REL(p.elastic.slope, al.E * kA0 / kL0, 0.01);   // N/mm of extension
    CHECK(p.yield.found);
    NEAR_REL(p.yield.y / kA0, al.rp02, 0.01);
    CHECK(p.proportionalLimit.found);
    NEAR_REL(p.proportionalLimit.y / kA0, Rp001(al), 0.02);
    CHECK(p.proportionalLimit.y < p.yield.y);
}

static void test_slack_at_the_start()
{
    // 0.4 mm of slack: the old fixed limits (samples 10 and 50) sit in it and give a far too low E-modulus
    const Material al{ 70000, 250, 12, 310 };
    const Curve c = MakeTest(al, 0.4, 0.0);
    const Placement p = Place(c, kL0);
    CHECK(p.elastic.ok);
    NEAR_REL(p.elastic.slope, al.E * kA0 / kL0, 0.01);
    CHECK(c[p.elastic.first].x > 0.4); // The window is past the slack
    // Rp0.2 counts the strain from where the elastic line starts, not from extension 0 (the slack is not strain)
    CHECK(p.yield.found);
    NEAR_REL(p.yield.y / kA0, al.rp02, 0.01);

    const AutoPoints::Line old = FitRange(c, 10, 50);
    CHECK(old.slope < 0.2 * p.elastic.slope);
}

static void test_noise_and_smoothing()
{
    const Material steel{ 200000, 355, 20, 470 };
    for (unsigned seed = 1; seed <= 20; ++seed)
    {
        const Curve c = MakeTest(steel, 0.2, 15.0, 2000, seed); // +-15 N noise on ~9400 N, smoothed (window 5)
        const Placement p = Place(c, kL0);
        CHECK(p.elastic.ok);
        NEAR_REL(p.elastic.slope, steel.E * kA0 / kL0, 0.03);
        CHECK(p.yield.found);
        NEAR_REL(p.yield.y / kA0, steel.rp02, 0.02);
    }
}

static void test_few_points()
{
    // A short test with 120 samples still works
    const Material al{ 70000, 250, 12, 310 };
    const Placement p = Place(MakeTest(al, 0.0, 0.0, 120), kL0);
    CHECK(p.elastic.ok && p.yield.found);
    NEAR_REL(p.yield.y / kA0, al.rp02, 0.03);
    CHECK(p.elastic.last - p.elastic.first + 1 >= Options{}.minPoints);
}

static void test_yield_plateau()
{
    // Mild steel: straight to 300 N/mm^2 (upper yield), drops to a plateau at 280, then hardens to 420
    Curve c;
    const double E = 200000;
    double strain = 0;
    for (int i = 0; i <= 300; ++i) { const double s = 300.0 * i / 300; strain = s / E; c.push_back({ (float)(strain * kL0), (float)(s * kA0) }); }
    for (int i = 1; i <= 10; ++i) { strain += 0.00005; c.push_back({ (float)(strain * kL0), (float)((300.0 - 2.0 * i) * kA0) }); }
    for (int i = 1; i <= 300; ++i) { strain += 0.0001; c.push_back({ (float)(strain * kL0), (float)(280.0 * kA0) }); }
    for (int i = 1; i <= 500; ++i) { strain += 0.0004; c.push_back({ (float)(strain * kL0), (float)((280.0 + 140.0 * std::sqrt(i / 500.0)) * kA0) }); }

    const Placement p = Place(c, kL0);
    CHECK(p.elastic.ok);
    NEAR_REL(p.elastic.slope, E * kA0 / kL0, 0.01);
    CHECK(p.yield.found);
    NEAR_REL(p.yield.y / kA0, 280.0, 0.02); // On the plateau (the lower yield)
}

static void test_brittle_and_bad_input()
{
    // Straight until it breaks: an E-modulus, but no yield point
    Curve brittle;
    for (int i = 0; i <= 200; ++i) brittle.push_back({ 0.001f * i, 10.0f * i });
    const Placement p = Place(brittle, kL0);
    CHECK(p.elastic.ok);
    CHECK(!p.yield.found && !p.proportionalLimit.found);

    CHECK(!Place(Curve{}, kL0).elastic.ok);
    CHECK(!Place(Curve{ { 0, 0 }, { 1, 1 }, { 2, 2 } }, kL0).elastic.ok); // Too few samples
    Curve flat;
    for (int i = 0; i < 100; ++i) flat.push_back({ 0.01f * i, 0.0f });
    CHECK(!Place(flat, kL0).elastic.ok); // No load at all

    // No gauge length (no dimensions entered): only the E-modulus limits
    const Material al{ 70000, 250, 12, 310 };
    const Placement noL0 = Place(MakeTest(al, 0.0, 0.0), 0.0);
    CHECK(noL0.elastic.ok && !noL0.yield.found);
    // 3-point bending: no offset yield either
    const Placement bending = Place(MakeTest(al, 0.0, 0.0), kL0, false);
    CHECK(bending.elastic.ok && !bending.yield.found);
}

static void test_fit_range()
{
    Curve line;
    for (int i = 0; i < 10; ++i) line.push_back({ (float)i, (float)(3.0 * i + 2.0) });
    const AutoPoints::Line l = FitRange(line, 0, 9);
    CHECK(l.ok);
    NEAR_REL(l.slope, 3.0, 1e-9);
    NEAR_REL(l.intercept, 2.0, 1e-9);
    NEAR_REL(l.r2, 1.0, 1e-12);
    NEAR_REL(l.ExtensionAtZeroLoad(), -2.0 / 3.0, 1e-9);
    CHECK(!FitRange(line, 5, 5).ok);
    CHECK(!FitRange(line, 0, 20).ok);
}

int main()
{
    test_fit_range();
    test_clean_aluminium();
    test_slack_at_the_start();
    test_noise_and_smoothing();
    test_few_points();
    test_yield_plateau();
    test_brittle_and_bad_input();

    if (g_failures == 0)
        std::printf("auto_points_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
