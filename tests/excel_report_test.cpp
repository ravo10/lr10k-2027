// Unit tests for ExcelReport (main/LloydLR10KTensile/ExcelReport.h): the numbers written to the Excel sheets.
//
//   g++ -std=c++17 -O1 -g -I main/LloydLR10KTensile tests/excel_report_test.cpp -o excel_report_test
//   ./excel_report_test
//
// Run automatically by .github/workflows/tests.yml.

#include "ExcelReport.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define NEAR_TOL(a, b, tol) CHECK(std::fabs((a) - (b)) <= (tol) * (1.0 + std::fabs(b)))
#define NEAR(a, b) NEAR_TOL(a, b, 1e-9)

using namespace ExcelReport;

static Report rectangularReport()
{
    Report r;
    r.specimen.crossSection = MaterialCalc::Rectangular;
    r.specimen.thickness = 2.0;
    r.specimen.width = 10.0;   // A_0 = 20 mm^2
    r.specimen.length = 50.0;  // L_0 = 50 mm
    for (int i = 0; i <= 100; ++i)
        r.data.push_back({ 0.5 + i * 0.05, i * 4.0 }); // 0.5 .. 5.5 mm, 0 .. 400 N
    return r;
}

static void test_load_extension_sheet()
{
    Report r = rectangularReport();
    r.zeroOffset = 0.5;
    const SheetRows rows = BuildSheet(r, Sheet::LoadExtension);
    CHECK(rows.curve.size() == r.data.size());
    NEAR(rows.curve[0].x, 0.0);
    NEAR(rows.curve[100].x, 5.0);
    NEAR(rows.curve[100].y, 400.0);
    CHECK(rows.points.empty());
}

static void test_stress_sheets()
{
    Report r = rectangularReport();
    r.zeroOffset = 0.5;
    // Last sample: 5 mm of 50 mm = 10 %, 400 N / 20 mm^2 = 20 N/mm^2
    const SheetRows eng = BuildSheet(r, Sheet::EngineeringStress);
    NEAR(eng.curve[100].x, 10.0);
    NEAR(eng.curve[100].y, 20.0);
    const SheetRows tru = BuildSheet(r, Sheet::TrueStress);
    NEAR(tru.curve[100].x, std::log(1.1) * 100.0);
    NEAR(tru.curve[100].y, 22.0);
}

static void test_crucial_points()
{
    Report r = rectangularReport();
    r.zeroOffset = 0.5;
    // UTS snapped onto the last sample, yield onto sample 50, E-limit not on a sample, unknown ID 9
    r.points = { { TestCalc::UltimateTensileStrength, { r.data[100].extension, 400.0 } },
                 { TestCalc::ModulusLower, { 1.234, 50.0 } },
                 { TestCalc::Yield, { r.data[50].extension, 200.0 } },
                 { 9, { r.data[10].extension, 40.0 } } };

    const SheetRows raw = BuildSheet(r, Sheet::LoadExtension);
    CHECK(raw.points.size() == 3);                    // unknown ID left out
    CHECK(raw.points[0].id == TestCalc::Yield);       // on-curve points in curve order
    CHECK(raw.points[1].id == TestCalc::UltimateTensileStrength);
    CHECK(raw.points[2].id == TestCalc::ModulusLower); // then the others (the old export dropped these)
    NEAR(raw.points[1].at.x, 5.0);                    // with the zero offset, like the curve
    NEAR(raw.points[1].at.y, 400.0);
    NEAR(raw.points[2].at.x, 1.234 - 0.5);

    const SheetRows eng = BuildSheet(r, Sheet::EngineeringStress);
    CHECK(eng.points.size() == 3);
    NEAR(eng.points[1].at.x, 10.0);
    NEAR(eng.points[1].at.y, 20.0);

    CHECK(BuildSheet(r, Sheet::TrueStress).points.empty()); // no points on the true stress sheet
}

static void test_dimensions_and_area()
{
    Report r = rectangularReport();
    CHECK(HasStressSheets(r));
    NEAR(AreaForExport(r), 20.0);

    // Circular specimens: the old export required thickness and width, so they got no report or stress sheets
    Report c;
    c.specimen.crossSection = MaterialCalc::Circular;
    c.specimen.diameter = 10.0;
    c.specimen.length = 50.0;
    c.storedArea = 7.854; // what older files stored for circular specimens (pi * d / 4)
    CHECK(HasStressSheets(c));
    NEAR(AreaForExport(c), M_PI * 25.0); // recomputed from the dimensions

    // No dimensions: stored area, else 1; no stress sheets
    Report none;
    none.storedArea = 12.5;
    CHECK(!HasStressSheets(none));
    NEAR(AreaForExport(none), 12.5);
    none.storedArea = 0.0;
    NEAR(AreaForExport(none), 1.0);
}

// --- The old AppMainExcel::write_worksheet_data numbers (before ExcelReport.h), for comparison.
// It worked in float (ImVec2) and wrote only points that sit exactly on a sample.
struct OldSheet { std::vector<Row> curve; std::vector<PointRow> points; };

static OldSheet oldWriteWorksheetData(const Report& r, Sheet sheet)
{
    OldSheet o;
    double A_0 = MaterialCalc::CrossSectionArea(r.specimen.crossSection, r.specimen.thickness, r.specimen.width, r.specimen.diameter);
    if (A_0 <= 0.0) A_0 = r.storedArea > 0 ? r.storedArea : 1.0;
    const double L_0 = r.specimen.length > 0 ? r.specimen.length : 1.0;

    for (const auto& s : r.data)
    {
        const float originalX = (float)s.extension;
        float x = originalX - (float)r.zeroOffset;
        float y = (float)s.load;
        float sx = x, sy = y;
        if (sheet != Sheet::LoadExtension) { sx = (float)(sx / L_0); sy = (float)(sy / A_0); }
        if (sheet == Sheet::TrueStress) { const double e = sx; sy = (float)(sy * (1.0 + e)); sx = (float)std::log(1.0 + e); }
        o.curve.push_back(sheet == Sheet::LoadExtension ? Row{ x, y } : Row{ sx * 100.0, sy });

        if (sheet == Sheet::TrueStress) continue;
        for (const auto& p : r.points)
        {
            double px = p.at.extension, py = p.at.load;
            const bool matches = (px == (double)originalX);
            px -= r.zeroOffset;
            if (sheet == Sheet::EngineeringStress) { px = px / L_0; py = py / A_0; }
            if (matches) o.points.push_back({ p.id, sheet == Sheet::EngineeringStress ? Row{ px * 100.0, py } : Row{ px, py } });
        }
    }
    return o;
}

static void test_same_numbers_as_the_old_export()
{
    std::mt19937 rng(2027);
    std::uniform_real_distribution<double> dim(0.5, 20.0), load(-20.0, 3000.0), off(0.0, 1.0), step(0.001, 0.05);
    std::uniform_int_distribution<int> shape(0, 1), count(0, 400), id(0, 6);

    for (int run = 0; run < 500; ++run)
    {
        Report r;
        r.specimen.crossSection = shape(rng);
        r.specimen.thickness = dim(rng);
        r.specimen.width = dim(rng);
        r.specimen.diameter = dim(rng);
        r.specimen.length = dim(rng) * 5;
        r.zeroOffset = off(rng);
        double x = 0.0;
        const int n = count(rng);
        for (int i = 0; i < n; ++i) { x += step(rng); r.data.push_back({ (double)(float)x, (double)(float)load(rng) }); } // the app stores floats
        // Points snapped to samples (the only kind the old export wrote), each on a different sample
        for (int k = 0; n > 0 && k < 5; ++k)
        {
            const auto& s = r.data[(size_t)(k * 997 + run) % r.data.size()];
            bool taken = false;
            for (const auto& p : r.points) taken |= (p.at.extension == s.extension);
            if (!taken) r.points.push_back({ id(rng), { s.extension, s.load } });
        }

        for (Sheet sheet : { Sheet::LoadExtension, Sheet::EngineeringStress, Sheet::TrueStress })
        {
            const SheetRows now = BuildSheet(r, sheet);
            const OldSheet old = oldWriteWorksheetData(r, sheet);
            CHECK(now.curve.size() == old.curve.size());
            CHECK(now.points.size() == old.points.size());
            if (now.curve.size() != old.curve.size() || now.points.size() != old.points.size())
                return;
            // Float vs double: the same to float precision (the sheets show 3 decimals)
            for (size_t i = 0; i < now.curve.size(); ++i)
            {
                NEAR_TOL(now.curve[i].x, old.curve[i].x, 1e-5);
                NEAR_TOL(now.curve[i].y, old.curve[i].y, 1e-5);
            }
            for (size_t i = 0; i < now.points.size(); ++i)
            {
                CHECK(now.points[i].id == old.points[i].id);
                NEAR(now.points[i].at.x, old.points[i].at.x);
                NEAR(now.points[i].at.y, old.points[i].at.y);
            }
        }
    }
}

int main()
{
    test_load_extension_sheet();
    test_stress_sheets();
    test_crucial_points();
    test_dimensions_and_area();
    test_same_numbers_as_the_old_export();

    if (g_failures == 0)
        std::printf("excel_report_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
