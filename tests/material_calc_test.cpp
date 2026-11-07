// Unit tests for the material property formulas (main/LloydLR10KTensile/MaterialCalc.h).
// Every expected value is worked out by hand in the comment next to it.
//
//   g++ -std=c++17 -Wall -Wextra -I main/LloydLR10KTensile tests/material_calc_test.cpp -o material_calc_test
//   ./material_calc_test
//
// Run automatically by .github/workflows/tests.yml.

#include "MaterialCalc.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace MaterialCalc;

static int g_failures = 0;

static void checkNear(double actual, double expected, double relTol, const char* what, int line)
{
    const double tol = relTol * (std::fabs(expected) > 1e-12 ? std::fabs(expected) : 1.0);
    if (!(std::fabs(actual - expected) <= tol))
    {
        std::printf("FAIL line %d: %s = %.10g, expected %.10g\n", line, what, actual, expected);
        ++g_failures;
    }
}
#define NEAR(actual, expected) checkNear((actual), (expected), 1e-9, #actual, __LINE__)
#define NEAR_TOL(actual, expected, tol) checkNear((actual), (expected), (tol), #actual, __LINE__)
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

static void test_cross_section_area()
{
    // Rectangle 2 mm x 10 mm = 20 mm^2
    NEAR(CrossSectionArea(Rectangular, 2.0, 10.0, 0.0), 20.0);
    // Round bar d = 10 mm: pi * 10^2 / 4 = 78.53981634 mm^2
    // (the app used to compute pi * d / 4 = 7.85 mm^2 here: 10x too small for d = 10)
    NEAR(CrossSectionArea(Circular, 0.0, 0.0, 10.0), 78.539816339744830962);
    // Round bar d = 6 mm: pi * 36 / 4 = 28.27433388 mm^2
    NEAR(CrossSectionArea(Circular, 0.0, 0.0, 6.0), 28.274333882308139146);
    // Missing dimensions -> 0 (caller must not divide by it)
    NEAR(CrossSectionArea(Rectangular, 0.0, 10.0, 0.0), 0.0);
    NEAR(CrossSectionArea(Circular, 2.0, 10.0, 0.0), 0.0);
}

static void test_engineering_and_true_values()
{
    // Round bar d = 10 mm, F = 10 000 N: sigma_e = 10000 / 78.5398 = 127.3239545 MPa
    const double A0 = CrossSectionArea(Circular, 0, 0, 10.0);
    const double sigmaE = EngineeringStress(10000.0, A0);
    NEAR(sigmaE, 127.32395447351627);

    // Gauge length 50 mm, elongation 5 mm: epsilon_e = 5 / 50 = 0.1 (10 %)
    const double epsE = EngineeringStrain(5.0, 50.0);
    NEAR(epsE, 0.1);

    // True strain: ln(1.1) = 0.0953101798
    NEAR(TrueStrain(epsE), 0.09531017980432486);
    // True stress: 127.3239545 * (1 + 0.1) = 140.0563499 MPa
    // (must use the ENGINEERING strain; the Excel export used the true strain here)
    NEAR(TrueStress(sigmaE, epsE), 140.05634992086790);
    // Using the true strain by mistake gives 139.459 MPa: make sure that is not what we compute
    CHECK(std::fabs(TrueStress(sigmaE, epsE) - sigmaE * (1.0 + TrueStrain(epsE))) > 0.5);

    // Zero strain: true == engineering
    NEAR(TrueStrain(0.0), 0.0);
    NEAR(TrueStress(250.0, 0.0), 250.0);
}

static void test_non_negative()
{
    NEAR(NonNegative(-0.0004), 0.0);   // slack before the specimen is loaded
    NEAR(NonNegative(-12.5), 0.0);
    NEAR(NonNegative(0.0), 0.0);
    NEAR(NonNegative(0.0123), 0.0123);
    NEAR(NonNegative(350.0), 350.0);
    CHECK(std::isnan(NonNegative(NAN))); // missing data stays visible as NaN
    NEAR(TrueStrain(NonNegative(-0.01)), 0.0);
}

static void test_youngs_modulus_fit()
{
    // Synthetic steel: E = 200 000 MPa (200 GPa) up to 0.4 % strain, then a much flatter
    // plastic part. The fit window 0.05 %..0.25 % must only see the elastic line.
    std::vector<double> strain, stress;
    for (int i = 0; i <= 1000; i++)
    {
        const double e = i * 0.00002; // 0 .. 2 %
        strain.push_back(e);
        stress.push_back(e <= 0.004 ? 200000.0 * e : 800.0 + 2000.0 * (e - 0.004));
    }
    LinearFit fit = FitLine(strain, stress, 0.0005, 0.0025);
    CHECK(fit.valid);
    CHECK(fit.points == 101); // 0.0005 .. 0.0025 in steps of 0.00002
    NEAR_TOL(fit.slope, 200000.0, 1e-9);   // MPa -> 200 GPa after / 1000
    NEAR_TOL(fit.intercept, 0.0, 1e-6);

    // Limits given in the wrong order give the same result
    LinearFit swapped = FitLine(strain, stress, 0.0025, 0.0005);
    NEAR_TOL(swapped.slope, 200000.0, 1e-9);

    // A line with an offset (e.g. slack before the specimen is gripped): slope is unaffected
    std::vector<double> stressOffset;
    for (double s : stress) stressOffset.push_back(s - 15.0);
    NEAR_TOL(FitLine(strain, stressOffset, 0.0005, 0.0025).slope, 200000.0, 1e-9);

    // Aluminium-like E = 70 GPa with +-0.5 MPa alternating noise: slope within 0.5 %
    std::vector<double> noisy;
    for (std::size_t i = 0; i < strain.size(); i++)
        noisy.push_back(70000.0 * strain[i] + ((i % 2) ? 0.5 : -0.5));
    NEAR_TOL(FitLine(strain, noisy, 0.0005, 0.0025).slope, 70000.0, 0.005);
}

static void test_fit_degenerate_cases()
{
    std::vector<double> x = { 0.001, 0.001, 0.001 };
    std::vector<double> y = { 1.0, 2.0, 3.0 };
    CHECK(!FitLine(x, y, 0.0, 1.0).valid);           // all x equal: no slope (used to divide by zero)

    std::vector<double> x2 = { 0.0, 0.01, 0.02 };
    std::vector<double> y2 = { 0.0, 10.0, 20.0 };
    CHECK(!FitLine(x2, y2, 0.1, 0.2).valid);          // no points in the window
    CHECK(FitLine(x2, y2, 0.1, 0.2).points == 0);
    CHECK(!FitLine(x2, y2, 0.005, 0.015).valid);      // only 1 point
    LinearFit two = FitLine(x2, y2, 0.0, 0.01);       // exactly 2 points: slope 10 / 0.01 = 1000
    CHECK(two.valid);
    NEAR(two.slope, 1000.0);

    std::vector<double> x3 = { 0.0, 0.001, NAN, 0.002 };
    std::vector<double> y3 = { 0.0, 1.0, 5.0, 2.0 };
    NEAR(FitLine(x3, y3, 0.0, 1.0).slope, 1000.0);    // NaN points are skipped
}

static void test_load_error_correction()
{
    // Live correction: +5 % load error -> measured loads x 1.05; -3 % -> x 0.97
    NEAR(LoadErrorFactor(5.0), 1.05);
    NEAR(LoadErrorFactor(-3.0), 0.97);
    NEAR(LoadErrorFactor(0.0), 1.0);
    NEAR(LoadErrorFactor(-100.0), 0.0); // rejected (would zero all loads)

    // Re-adjusting a finished test 0 -> 5 -> 10 -> 0 % must end exactly at the original load
    double load = 1000.0;
    load *= LoadErrorRescaleFactor(0.0, 5.0);   // 1050
    NEAR(load, 1050.0);
    load *= LoadErrorRescaleFactor(5.0, 10.0);  // 1100 (the old formula gave 1102.5)
    NEAR(load, 1100.0);
    load *= LoadErrorRescaleFactor(10.0, 0.0);  // back to 1000 (the old formula gave 992.25)
    NEAR(load, 1000.0);

    // Same with negative values and many steps
    load = 1234.5;
    const double steps[] = { -2.0, 7.5, -10.0, 3.3, 0.0 };
    double applied = 0.0;
    for (double s : steps) { load *= LoadErrorRescaleFactor(applied, s); applied = s; }
    NEAR(load, 1234.5);

    NEAR(LoadErrorRescaleFactor(5.0, 5.0), 1.0);        // no change
    NEAR(LoadErrorRescaleFactor(0.0, -100.0), 0.0);     // rejected
    NEAR(LoadErrorRescaleFactor(-100.0, 0.0), 0.0);     // rejected
}

static void test_three_point_bending()
{
    // Span L = 64 mm, width w = 10 mm, thickness t = 4 mm (ISO 178 style specimen)
    // Max moment: M = 100 N * 64 / 4 = 1600 N*mm
    NEAR(BendingMoment3Point(100.0, 64.0), 1600.0);
    // Flexural stress: 3 * 100 * 64 / (2 * 10 * 4^2) = 19200 / 320 = 60 MPa
    NEAR(FlexuralStress3Point(100.0, 64.0, 10.0, 4.0), 60.0);
    // Flexural modulus from slope 50 N/mm: 50 * 64^3 / (4 * 10 * 4^3) = 13107200 / 2560 = 5120 MPa
    NEAR(FlexuralModulus3Point(50.0, 64.0, 10.0, 4.0), 5120.0);
}

static void test_unit_factors()
{
    // The app's unit table: 1 mm = 0.0393700787 in, 1 N = 0.2248089431 lbf.
    const double mmToIn = 0.0393700787;
    const double nToLbf = 0.2248089431;

    // Stress computed from converted force and area must equal MPa * 145.0377 (1 MPa = 145.0377 psi)
    const double forceN = 10000.0, areaMm2 = 78.539816339744830962;
    const double mpa = EngineeringStress(forceN, areaMm2);
    const double psi = EngineeringStress(forceN * nToLbf, areaMm2 * mmToIn * mmToIn);
    NEAR_TOL(psi / mpa, 145.0377, 1e-6);

    // Strain is dimensionless: the same in mm and in
    NEAR(EngineeringStrain(5.0 * mmToIn, 50.0 * mmToIn), EngineeringStrain(5.0, 50.0));

    // Young's modulus: SI result in MPa is divided by 1000 for GPa; imperial result is already psi
    NEAR_TOL(200000.0 / 1000.0, 200.0, 1e-12);
}

int main()
{
    test_cross_section_area();
    test_engineering_and_true_values();
    test_non_negative();
    test_youngs_modulus_fit();
    test_fit_degenerate_cases();
    test_load_error_correction();
    test_three_point_bending();
    test_unit_factors();

    if (g_failures == 0)
        std::printf("material_calc_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
