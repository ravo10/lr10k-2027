// Unit tests for TestCalc (main/LloydLR10KTensile/TestCalc.h): the calculations for one test.
//
//   g++ -std=c++17 -O1 -g -I main/LloydLR10KTensile tests/test_calc_test.cpp -o test_calc_test
//   ./test_calc_test
//
// Run automatically by .github/workflows/tests.yml.

#include "TestCalc.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define NEAR(a, b) CHECK(std::fabs((a) - (b)) <= 1e-9 * (1.0 + std::fabs(b)))

using namespace TestCalc;

static Specimen rectangular(double t, double w, double l)
{
    Specimen s;
    s.crossSection = MaterialCalc::Rectangular;
    s.thickness = t;
    s.width = w;
    s.length = l;
    return s;
}

// A linear-elastic test: load = k * extension, from 0 to 2 mm in 0.01 mm steps
static std::vector<Sample> linearData(double k, double maxExtension = 2.0)
{
    std::vector<Sample> data;
    for (double x = 0.0; x <= maxExtension + 1e-12; x += 0.01)
        data.push_back({ x, k * x });
    return data;
}

static void test_points_hand_calculated()
{
    // 2 x 10 mm (A_0 = 20 mm^2), L_0 = 50 mm. UTS at 5 mm, 400 N: 20 N/mm^2 at 10 % strain
    const Specimen s = rectangular(2.0, 10.0, 50.0);
    std::vector<MarkedPoint> points = { { UltimateTensileStrength, { 5.0, 400.0 } }, { Fracture, { 6.0, 300.0 } } };
    const TensileResults r = CalculateTensile(s, {}, points, 0.0);

    CHECK(r.found[UltimateTensileStrength] && r.found[Fracture]);
    CHECK(!r.found[ProportionalLimit] && !r.found[Yield]);
    NEAR(r.nominal[UltimateTensileStrength].strain, 0.1);
    NEAR(r.nominal[UltimateTensileStrength].stress, 20.0);
    NEAR(r.truePoint[UltimateTensileStrength].strain, std::log(1.1));
    NEAR(r.truePoint[UltimateTensileStrength].stress, 22.0);
    NEAR(r.nominal[Fracture].strain, 0.12);
    NEAR(r.nominal[Fracture].stress, 15.0);
    NEAR(r.nominal[Yield].stress, 0.0);
    CHECK(!r.modulusCalculated); // no data
}

static void test_zero_offset()
{
    // 1 mm of slack: the UTS at 6 mm is 5 mm of real extension
    const Specimen s = rectangular(2.0, 10.0, 50.0);
    const TensileResults r = CalculateTensile(s, {}, { { UltimateTensileStrength, { 6.0, 400.0 } } }, 1.0);
    NEAR(r.nominal[UltimateTensileStrength].strain, 0.1);
}

static void test_uses_only_the_specimen_given()
{
    // The bug this replaces: the same test calculated with other (global) dimensions gave other results.
    // Here the result depends only on the specimen passed in.
    std::vector<MarkedPoint> points = { { UltimateTensileStrength, { 5.0, 400.0 } } };
    const TensileResults a = CalculateTensile(rectangular(2.0, 10.0, 50.0), {}, points, 0.0);
    const TensileResults b = CalculateTensile(rectangular(4.0, 10.0, 100.0), {}, points, 0.0);
    NEAR(a.nominal[UltimateTensileStrength].stress, 20.0);
    NEAR(b.nominal[UltimateTensileStrength].stress, 10.0);
    NEAR(b.nominal[UltimateTensileStrength].strain, 0.05);

    // Circular: d = 10 mm, A_0 = 78.54 mm^2
    Specimen c;
    c.crossSection = MaterialCalc::Circular;
    c.diameter = 10.0;
    c.length = 50.0;
    const TensileResults r = CalculateTensile(c, {}, points, 0.0);
    NEAR(r.nominal[UltimateTensileStrength].stress, 400.0 / (M_PI * 25.0));
}

static void test_missing_dimensions_do_not_divide_by_zero()
{
    const TensileResults r = CalculateTensile(Specimen{}, linearData(100.0), { { UltimateTensileStrength, { 5.0, 400.0 } } }, 0.0);
    NEAR(r.nominal[UltimateTensileStrength].stress, 400.0); // A_0 = 1
    NEAR(r.nominal[UltimateTensileStrength].strain, 5.0);   // L_0 = 1
    CHECK(std::isfinite(r.modulusFit.slope));
}

static void test_modulus()
{
    // load = 1000 N/mm * x; A_0 = 20 mm^2, L_0 = 50 mm: stress = 50 x, strain = x / 50 -> E = 2500 N/mm^2
    const Specimen s = rectangular(2.0, 10.0, 50.0);
    TensileResults r = CalculateTensile(s, linearData(1000.0), {}, 0.0);
    CHECK(r.modulusCalculated);
    CHECK(r.modulusFit.valid);
    NEAR(r.modulusLowerStrain, 0.0005);
    NEAR(r.modulusUpperStrain, 0.0025);
    NEAR(r.modulusFit.slope, 2500.0);

    // Limits placed by the user (IDs 5 and 6) at 0.5 and 1.5 mm: 1 % .. 3 % strain
    r = CalculateTensile(s, linearData(1000.0), { { ModulusLower, { 0.5, 0.0 } }, { ModulusUpper, { 1.5, 0.0 } } }, 0.0);
    NEAR(r.modulusLowerStrain, 0.01);
    NEAR(r.modulusUpperStrain, 0.03);
    NEAR(r.modulusFit.slope, 2500.0);

    // Too little data: no modulus
    const std::vector<Sample> all = linearData(1000.0);
    const std::vector<Sample> tenSamples(all.begin(), all.begin() + 10);
    CHECK(!CalculateTensile(s, tenSamples, {}, 0.0).modulusCalculated);
}

// --- The old AppCalculator::CalculateStressPoints formulas (before TestCalc.h), for comparison ---
struct OldResults { double nom[4][2]{}; double tru[4][2]{}; MaterialCalc::LinearFit fit; };

static OldResults oldCalculateStressPoints(int areaOption, double T_0, double W_0, double length, double D_0,
    const std::vector<Sample>& data, const std::vector<MarkedPoint>& points, double offset)
{
    OldResults o;
    const double L_0 = length > 0 ? length : 1.0;
    double A_0 = MaterialCalc::CrossSectionArea(areaOption, T_0, W_0, D_0);
    if (A_0 <= 0.0) A_0 = 1.0;
    for (const auto& p : points)
    {
        if (p.id >= 0 && p.id <= 3)
        {
            o.nom[p.id][0] = (p.at.extension - offset) / L_0;
            o.nom[p.id][1] = p.at.load / A_0;
            o.tru[p.id][0] = std::log(1.0 + o.nom[p.id][0]);
            o.tru[p.id][1] = o.nom[p.id][1] * (1.0 + o.nom[p.id][0]);
        }
    }
    if (data.size() > 10)
    {
        double e_lower = 0.0005, e_upper = 0.0025;
        for (const auto& p : points)
        {
            if (p.id == 5) e_lower = (p.at.extension - offset) / L_0;
            if (p.id == 6) e_upper = (p.at.extension - offset) / L_0;
        }
        std::vector<double> strain, stress;
        for (const auto& d : data)
        {
            double x = d.extension - offset;
            x = x > 0 ? x : 0.0;
            const double sigma = x == 0 ? 0.0 : d.load;
            strain.push_back(x / L_0);
            stress.push_back(sigma / A_0);
        }
        o.fit = MaterialCalc::FitLine(strain, stress, e_lower, e_upper);
    }
    return o;
}

static void test_same_as_the_old_calculator_for_random_tests()
{
    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> dim(0.0, 20.0), ext(-1.0, 10.0), load(-50.0, 2000.0), off(0.0, 1.5);
    std::uniform_int_distribution<int> id(-1, 7), shape(0, 1), count(0, 300);

    for (int run = 0; run < 2000; ++run)
    {
        Specimen s;
        s.crossSection = shape(rng);
        s.thickness = run % 17 == 0 ? 0.0 : dim(rng);
        s.width = dim(rng);
        s.length = run % 13 == 0 ? 0.0 : dim(rng);
        s.diameter = dim(rng);
        const double offset = off(rng);

        std::vector<Sample> data(count(rng));
        double x = 0.0;
        for (auto& d : data) { x += 0.03; d = { x, load(rng) }; }
        std::vector<MarkedPoint> points(run % 8);
        for (auto& p : points) p = { id(rng), { ext(rng), load(rng) } };

        const TensileResults r = CalculateTensile(s, data, points, offset);
        const OldResults o = oldCalculateStressPoints(s.crossSection, s.thickness, s.width, s.length, s.diameter, data, points, offset);

        for (int i = 0; i < 4; ++i)
        {
            if (std::isnan(o.tru[i][0])) { CHECK(std::isnan(r.truePoint[i].strain)); continue; } // log of a negative number
            NEAR(r.nominal[i].strain, o.nom[i][0]);
            NEAR(r.nominal[i].stress, o.nom[i][1]);
            NEAR(r.truePoint[i].strain, o.tru[i][0]);
            NEAR(r.truePoint[i].stress, o.tru[i][1]);
        }
        CHECK(r.modulusCalculated == (data.size() > 10));
        CHECK(r.modulusFit.valid == o.fit.valid);
        CHECK(r.modulusFit.points == o.fit.points);
        if (o.fit.valid) NEAR(r.modulusFit.slope, o.fit.slope);
    }
}

static void test_bending()
{
    // t = 4, w = 10, span = 64 mm. Max 500 N at 3 mm (0.5 mm slack)
    const Specimen s = rectangular(4.0, 10.0, 64.0);
    const BendingResults r = CalculateBending3Point(s, linearData(200.0, 4.0), {}, 0.5, { 3.0, 500.0 }, 0.5, 2.0);
    NEAR(r.maxLoad, 500.0);
    NEAR(r.maxTravel, 2.5);
    NEAR(r.bendingMoment, 500.0 * 64.0 / 4.0);                       // 8000 N mm
    NEAR(r.flexuralStress, 3.0 * 500.0 * 64.0 / (2.0 * 10.0 * 16.0)); // 300 N/mm^2
    CHECK(r.modulusCalculated && r.loadPerTravel.valid);
    NEAR(r.loadPerTravel.slope, 200.0);
    NEAR(r.flexuralModulus, 200.0 * 64.0 * 64.0 * 64.0 / (4.0 * 10.0 * 64.0)); // 20 480 N/mm^2

    // Limits placed by the user are read straight off the travel axis
    const BendingResults l = CalculateBending3Point(s, linearData(200.0, 4.0), { { ModulusLower, { 1.0, 0 } }, { ModulusUpper, { 3.0, 0 } } }, 0.5, { 3.0, 500.0 }, 0.5, 2.0);
    NEAR(l.modulusLowerTravel, 1.0);
    NEAR(l.modulusUpperTravel, 3.0);

    // Window with no data: modulus 0, not NaN
    const BendingResults e = CalculateBending3Point(s, linearData(200.0, 4.0), {}, 0.0, { 3.0, 500.0 }, 10.0, 20.0);
    CHECK(!e.loadPerTravel.valid);
    NEAR(e.flexuralModulus, 0.0);

    // Missing dimensions count as 1
    const BendingResults m = CalculateBending3Point(Specimen{}, {}, {}, 0.0, { 3.0, 500.0 }, 0.5, 2.0);
    NEAR(m.bendingMoment, 125.0);
    NEAR(m.flexuralStress, 750.0);
    CHECK(!m.modulusCalculated);
}

static void test_specimen_for_new_test()
{
    Specimen placeholder; // a new tab
    placeholder.thickness = placeholder.width = placeholder.length = placeholder.diameter = 0.01;
    const Specimen local = rectangular(3.0, 12.0, 80.0);
    const Specimen global = rectangular(2.0, 10.0, 50.0);

    CHECK(IsPlaceholder(placeholder));
    CHECK(!IsPlaceholder(local));
    CHECK(HasDimensions(local));
    CHECK(!HasDimensions(Specimen{}));
    CHECK(!HasDimensions(rectangular(2.0, 0.0, 50.0)));
    Specimen circ;
    circ.crossSection = MaterialCalc::Circular;
    circ.length = 50.0;
    CHECK(!HasDimensions(circ));
    circ.diameter = 8.0;
    CHECK(HasDimensions(circ));

    // Entered local values win, unless "Always load from global values" is on
    NEAR(SpecimenForNewTest(false, local, global).length, 80.0);
    NEAR(SpecimenForNewTest(true, local, global).length, 50.0);
    // New tab (placeholders) or incomplete local values: the global values
    NEAR(SpecimenForNewTest(false, placeholder, global).length, 50.0);
    NEAR(SpecimenForNewTest(false, rectangular(3.0, 0.0, 80.0), global).length, 50.0);
}

int main()
{
    test_points_hand_calculated();
    test_zero_offset();
    test_uses_only_the_specimen_given();
    test_missing_dimensions_do_not_divide_by_zero();
    test_modulus();
    test_same_as_the_old_calculator_for_random_tests();
    test_bending();
    test_specimen_for_new_test();

    if (g_failures == 0)
        std::printf("test_calc_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
