// Writes sample Excel reports with ExcelWriter (main/LloydLR10KTensile/ExcelWriter.cpp) for checking.
//
//   g++ -std=c++20 -I main/LloydLR10KTensile tests/excel_writer_check.cpp main/LloydLR10KTensile/ExcelWriter.cpp -lxlsxwriter -o excel_writer_check
//   ./excel_writer_check out_dir
//   python3 tests/check_excel_reports.py out_dir
//
// Run by .github/workflows/tests.yml (libxlsxwriter-dev from Ubuntu); the files are uploaded as an artifact.

#include "ExcelWriter.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace ExcelReport;

static Report baseReport()
{
    Report r;
    r.date = "07.11.2026 18:30";
    r.description = "Sample report written by tests/excel_writer_check.cpp";
    r.appVersion = "test";
    r.exportedAt = "2026-11-07 18:30";
    r.crossheadSpeed = 10.0;
    r.jogSpeed = 50.0;
    r.loadErrorPercent = -2.0;
    r.samplingRateHz = 21.0;
    return r;
}

// A tensile curve: linear to 300 N at 1.5 mm, max 400 N at 5 mm, fracture at 6 mm
static void addTensileCurve(Report& r, double slack)
{
    for (int i = 0; i <= 600; ++i)
    {
        const double x = i * 0.01;
        double load = x < 1.5 ? 200.0 * x : 300.0 + 100.0 * std::sin((x - 1.5) / 3.5 * 1.5707963);
        if (x > 5.0) load = 400.0 - 60.0 * (x - 5.0);
        r.data.push_back({ (double)(float)(x + slack), (double)(float)load });
    }
}

static bool save(const Report& r, const std::string& dir, const char* name)
{
    const ExcelWriter::Result result = ExcelWriter::Save(r, (dir + "/" + name).c_str());
    std::printf("%s: %s %s\n", result.fileName.c_str(), result.ok ? "ok" : "FAILED", result.error.c_str());
    return result.ok;
}

int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    bool ok = true;

    // 1. Tension, rectangular 2 x 10 mm, L0 = 50 mm, 0.5 mm slack; UTS and fracture placed, yield not
    {
        Report r = baseReport();
        r.specimen.crossSection = MaterialCalc::Rectangular;
        r.specimen.thickness = 2.0;
        r.specimen.width = 10.0;
        r.specimen.length = 50.0;
        r.zeroOffset = 0.5;
        addTensileCurve(r, 0.5);
        r.points = { { TestCalc::UltimateTensileStrength, r.data[500] },
                     { TestCalc::Fracture, r.data[600] },
                     { TestCalc::ModulusLower, { 0.8, 60.0 } } };
        r.nominal[TestCalc::UltimateTensileStrength] = { 0.1, 20.0 };
        r.nominal[TestCalc::Fracture] = { 0.11, 17.0 };
        r.youngsModulus = 0.5;
        ok &= save(r, dir, "tension_rectangular.xlsx");
    }

    // 2. Tension, circular d = 10 mm (used to get no report or stress sheets)
    {
        Report r = baseReport();
        r.specimen.crossSection = MaterialCalc::Circular;
        r.specimen.diameter = 10.0;
        r.specimen.length = 50.0;
        addTensileCurve(r, 0.0);
        r.points = { { TestCalc::UltimateTensileStrength, r.data[500] } };
        r.nominal[TestCalc::UltimateTensileStrength] = { 0.1, 400.0 / (M_PI * 25.0) };
        ok &= save(r, dir, "tension_circular.xlsx");
    }

    // 3. 3-point bending: results section, no stress sheets
    {
        Report r = baseReport();
        r.tension = false;
        r.kind = TestKind::Bending3Point;
        r.specimen.thickness = 4.0;
        r.specimen.width = 10.0;
        r.specimen.length = 64.0;
        addTensileCurve(r, 0.0);
        r.bending = { 400.0, 5.0, 6400.0, 240.0 };
        r.youngsModulus = 3.2;
        ok &= save(r, dir, "bending_3point.xlsx");
    }

    // 4. No dimensions: report with a note, load-extension graph only
    {
        Report r = baseReport();
        addTensileCurve(r, 0.0);
        ok &= save(r, dir, "no_dimensions.xlsx");
    }

    // 5. No data at all: must still be a valid file
    {
        Report r = baseReport();
        r.specimen.thickness = 2.0;
        r.specimen.width = 10.0;
        r.specimen.length = 50.0;
        ok &= save(r, dir, "no_data.xlsx");
    }

    return ok ? 0 : 1;
}
