/**

    @file      ExcelReport.h
    @brief     Everything the Excel export writes, as plain data, and the numbers for each sheet.
    @details   Portable (no GUI, Windows or libxlsxwriter code) and unit tested (tests/excel_report_test.cpp).
               ExcelExport.cpp fills a Report from a test tab and only does the formatting/writing.

**/
#pragma once

#include "TestCalc.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>
#include <string>
#include <vector>

namespace ExcelReport
{
    //! Unit labels for the test's unit system.
    struct Units
    {
        bool si = true;
        std::string length = "mm";        //!< e.g. mm / in
        std::string force = "N";          //!< e.g. N / lbf
        std::string speed = "mm/min";
        std::string stress = "N/mm^2";    //!< In table headers and the report
        std::string stressAxis = "MPa";   //!< On the chart axis
        std::string modulus = "GPa";      //!< The E-modulus is shown as GPa with SI units, psi with imperial units
    };

    enum class TestKind { Tension, Compression, Bending3Point };

    //! 3-point bending results calculated in the app (TestCalc::BendingResults).
    struct BendingValues
    {
        double maxLoad = 0.0;
        double maxTravel = 0.0;
        double bendingMoment = 0.0;
        double flexuralStress = 0.0;
    };

    //! One finished test: all the data the export needs.
    struct Report
    {
        std::string date;
        std::string description;
        std::string appVersion;  //!< Written on the setup sheet
        std::string exportedAt;  //!< e.g. "2026-11-07 18:30"
        Units units;

        bool tension = true;     //!< (kept for older callers; kind says the same and more)
        TestKind kind = TestKind::Tension;
        TestCalc::Specimen specimen;
        double storedArea = 0.0;  //!< The area saved with the test (only used if the dimensions give none)
        double zeroOffset = 0.0;  //!< The test's zero point on the extension axis (slack)

        // Results calculated in the app (TestCalc)
        double youngsModulus = 0.0;  //!< In units.modulus (the flexural modulus for 3-point bending)
        BendingValues bending;       //!< Only for TestKind::Bending3Point
        TestCalc::StressStrain nominal[TestCalc::TensileResults::kPoints]; //!< Prop. limit, yield, UTS, fracture

        // Machine settings
        double crossheadSpeed = 0.0;
        double jogSpeed = 0.0;
        double loadErrorPercent = 0.0;
        double samplingRateHz = 0.0;

        std::vector<TestCalc::Sample> data;          //!< Extension/load as recorded (before the zero offset)
        std::vector<TestCalc::MarkedPoint> points;   //!< Crucial points as placed on the curve (before the zero offset)
    };

    //! Stress/strain needs the specimen dimensions.
    inline bool HasDimensions(const Report& r) { return TestCalc::HasDimensions(r.specimen); }

    //! The engineering/true stress sheets: tension or plain compression with dimensions. (3-point bending has
    //! no uniform stress/strain: those sheets used to be written for it anyway.)
    inline bool HasStressSheets(const Report& r) { return r.kind != TestKind::Bending3Point && HasDimensions(r); }

    //! Name of a crucial point ID (same as in the app), or nullptr for an unknown ID.
    inline const char* PointName(int id)
    {
        static const char* const names[] = { "Proportional Limit", "Yielding Strength", "Ultimate Tensile Strength", "Fracture", "Other", "E-Lower Bound", "E-Upper Bound" };
        return (id >= 0 && id < 7) ? names[id] : nullptr;
    }

    //! Short name of a crucial point ID for chart labels (same as in the app), or nullptr for an unknown ID.
    inline const char* PointShortName(int id)
    {
        static const char* const names[] = { "Prop. Limit", "Yield Str.", "UTS", "Fracture", "Other", "E-Lower", "E-Upper" };
        return (id >= 0 && id < 7) ? names[id] : nullptr;
    }

    //! True if the user placed a point with this ID.
    inline bool HasPoint(const Report& r, int id)
    {
        for (const TestCalc::MarkedPoint& p : r.points)
            if (p.id == id) return true;
        return false;
    }

    inline const char* TestKindText(TestKind kind)
    {
        switch (kind)
        {
        case TestKind::Compression:   return "Compression";
        case TestKind::Bending3Point: return "Compression (3-point bending)";
        default:                      return "Tension";
        }
    }

    //! A_0 for the export: from the dimensions (older files have a wrong stored area for circular specimens),
    //! else the stored area, else 1.
    inline double AreaForExport(const Report& r)
    {
        const double a = MaterialCalc::CrossSectionArea(r.specimen.crossSection, r.specimen.thickness, r.specimen.width, r.specimen.diameter);
        if (a > 0.0) return a;
        return r.storedArea > 0.0 ? r.storedArea : 1.0;
    }

    // -------------------------------------------------------------------------
    // Report and setup sheets: label / value / unit lines
    // -------------------------------------------------------------------------

    struct Line
    {
        std::string label;
        bool isNumber = false;
        double number = 0.0;
        std::string text;     //!< When not a number ("-" = not available)
        std::string unit;
        int decimals = 3;
    };

    inline Line NumberLine(std::string label, double value, std::string unit, int decimals = 3)
    {
        Line l;
        l.label = std::move(label);
        l.isNumber = true;
        l.number = value;
        l.unit = std::move(unit);
        l.decimals = decimals;
        return l;
    }

    inline Line TextLine(std::string label, std::string text, std::string unit = "")
    {
        Line l;
        l.label = std::move(label);
        l.text = std::move(text);
        l.unit = std::move(unit);
        return l;
    }

    struct Section
    {
        std::string title;
        std::vector<Line> lines;
    };

    //! One row of the crucial points table (tension/compression).
    struct PointResult
    {
        int id = -1;
        std::string name;
        bool placed = false;          //!< False: not placed on the curve (shown as "-", used to be 0.000)
        double stress = 0.0;          //!< Engineering
        double strainPercent = 0.0;
        double trueStress = 0.0;
        double trueStrainPercent = 0.0;
    };

    //! The proportional limit, yield, UTS and fracture rows, with true stress/strain from the engineering values.
    inline std::vector<PointResult> PointResults(const Report& r)
    {
        std::vector<PointResult> rows;
        for (int id = 0; id < TestCalc::TensileResults::kPoints; ++id)
        {
            PointResult row;
            row.id = id;
            row.name = PointName(id);
            if (r.kind == TestKind::Compression && id == TestCalc::UltimateTensileStrength)
                row.name = "Ultimate Compressive Strength";
            row.placed = HasPoint(r, id);
            if (row.placed)
            {
                const TestCalc::StressStrain& n = r.nominal[id];
                row.stress = n.stress;
                row.strainPercent = n.strain * 100.0;
                row.trueStress = MaterialCalc::TrueStress(n.stress, n.strain);
                row.trueStrainPercent = MaterialCalc::TrueStrain(n.strain) * 100.0;
            }
            rows.push_back(row);
        }
        return rows;
    }

    /**
        @brief  The test date for the report: "Sun Nov  8 15:02:26 2026" (as saved by the app, std::ctime) becomes
                "2026-11-08 15:02:26". Any other text is kept, without a trailing newline or doubled spaces.
    **/
    inline std::string TidyDate(const std::string& date)
    {
        std::string text;
        for (char c : date)
        {
            if (c == '\n' || c == '\r') c = ' ';
            if (c == ' ' && (text.empty() || text.back() == ' ')) continue;
            text += c;
        }
        while (!text.empty() && text.back() == ' ') text.pop_back();

        static const char* const months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        char weekday[4] = "", month[4] = "";
        int day = 0, hour = 0, minute = 0, second = 0, year = 0, used = 0;
        if (std::sscanf(text.c_str(), "%3s %3s %d %d:%d:%d %d%n", weekday, month, &day, &hour, &minute, &second, &year, &used) == 7
            && used == (int)text.size() && day >= 1 && day <= 31 && hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 61 && year > 1900 && year < 10000)
        {
            for (int m = 0; m < 12; ++m)
            {
                if (std::strcmp(month, months[m]) != 0) continue;
                char out[64];
                std::snprintf(out, sizeof(out), "%04d-%02d-%02d %02d:%02d:%02d", year, m + 1, day, hour, minute, second);
                return out;
            }
        }
        return text;
    }

    //! The sample with the largest load (by size, so compression works too). Extension minus the zero offset.
    inline bool MaximumLoad(const Report& r, double& load, double& extension)
    {
        if (r.data.empty()) return false;
        const TestCalc::Sample* best = &r.data[0];
        for (const TestCalc::Sample& s : r.data)
            if (std::fabs(s.load) > std::fabs(best->load)) best = &s;
        load = best->load;
        extension = best->extension - r.zeroOffset;
        return true;
    }

    //! One of the key results shown at the top of the report.
    struct KeyResult
    {
        std::string label;
        bool known = false;   //!< False: shown as "-" (point not placed, nothing calculated)
        double value = 0.0;
        std::string unit;
        int decimals = 2;
    };

    /**
        @brief  The key results at the top of the report (up to 4).
        @details Tension/compression with dimensions: E-modulus, strength (UTS), elongation at fracture, maximum load.
                 3-point bending: maximum load, flexural strength, flexural modulus, travel at maximum load.
                 Without dimensions: maximum load and the extension there.
    **/
    inline std::vector<KeyResult> KeyResults(const Report& r)
    {
        const Units& u = r.units;
        std::vector<KeyResult> out;
        double maxLoad = 0.0, maxExtension = 0.0;
        const bool hasData = MaximumLoad(r, maxLoad, maxExtension);
        auto add = [&](std::string label, bool known, double value, std::string unit, int decimals) {
            out.push_back({ std::move(label), known, known ? value : 0.0, std::move(unit), decimals });
        };

        if (r.kind == TestKind::Bending3Point)
        {
            const bool calculated = r.bending.maxLoad != 0.0 || r.bending.flexuralStress != 0.0;
            add("Maximum load", calculated, r.bending.maxLoad, u.force, 1);
            add("Flexural strength", calculated, r.bending.flexuralStress, u.stress, 2);
            add("Flexural modulus", r.youngsModulus != 0.0, r.youngsModulus, u.modulus, 2);
            add("Travel at maximum load", calculated, r.bending.maxTravel, u.length, 3);
            return out;
        }
        if (HasDimensions(r))
        {
            add("Young's modulus", r.youngsModulus != 0.0, r.youngsModulus, u.modulus, 2);
            add(r.kind == TestKind::Compression ? "Compressive strength" : "Tensile strength",
                HasPoint(r, TestCalc::UltimateTensileStrength), r.nominal[TestCalc::UltimateTensileStrength].stress, u.stress, 2);
            add(r.kind == TestKind::Compression ? "Strain at fracture" : "Elongation at fracture",
                HasPoint(r, TestCalc::Fracture), r.nominal[TestCalc::Fracture].strain * 100.0, "%", 2);
            add("Maximum load", hasData, maxLoad, u.force, 1);
            return out;
        }
        add("Maximum load", hasData, maxLoad, u.force, 1);
        add("Extension at maximum load", hasData, maxExtension, u.length, 3);
        return out;
    }

    //! The report sheet above the points table: test, specimen and (for E-modulus / 3-point bending) results.
    inline std::vector<Section> ReportSections(const Report& r)
    {
        const Units& u = r.units;
        std::vector<Section> sections;

        Section test{ "Test", {} };
        test.lines.push_back(TextLine("Test date", TidyDate(r.date)));
        test.lines.push_back(TextLine("Test type", TestKindText(r.kind)));
        test.lines.push_back(TextLine("Units", u.si ? "SI" : "English Engineering"));
        sections.push_back(test);

        const bool bending = r.kind == TestKind::Bending3Point;
        Section specimen{ "Specimen", {} };
        if (bending || r.specimen.crossSection == MaterialCalc::Rectangular)
        {
            specimen.lines.push_back(TextLine("Cross-section", "Rectangular"));
            specimen.lines.push_back(NumberLine("Thickness", r.specimen.thickness, u.length));
            specimen.lines.push_back(NumberLine("Width", r.specimen.width, u.length));
        }
        else
        {
            specimen.lines.push_back(TextLine("Cross-section", "Circular"));
            specimen.lines.push_back(NumberLine("Diameter", r.specimen.diameter, u.length));
        }
        specimen.lines.push_back(NumberLine(bending ? "Support span (L)" : "Gauge length (L0)", r.specimen.length, u.length));
        if (!bending)
            specimen.lines.push_back(NumberLine("Cross-sectional area (A0)", HasDimensions(r) ? AreaForExport(r) : 0.0, u.length + "^2"));
        if (r.zeroOffset != 0.0)
            specimen.lines.push_back(NumberLine("Zero point offset (slack)", r.zeroOffset, u.length));
        sections.push_back(specimen);

        Section results{ "Results", {} };
        if (bending)
        {
            results.lines.push_back(NumberLine("Maximum load", r.bending.maxLoad, u.force));
            results.lines.push_back(NumberLine("Travel at maximum load", r.bending.maxTravel, u.length));
            results.lines.push_back(NumberLine("Maximum bending moment", r.bending.bendingMoment, u.force + "*" + u.length));
            results.lines.push_back(NumberLine("Flexural strength", r.bending.flexuralStress, u.stress));
            results.lines.push_back(NumberLine("Flexural modulus", r.youngsModulus, u.modulus));
        }
        else if (HasDimensions(r))
        {
            results.lines.push_back(NumberLine("Young's modulus (E)", r.youngsModulus, u.modulus));
        }
        else
        {
            results.lines.push_back(TextLine("Stress, strain and E-modulus", "Not calculated: enter the specimen dimensions in the Calculator"));
        }
        sections.push_back(results);
        return sections;
    }

    //! The setup sheet.
    inline std::vector<Line> SetupLines(const Report& r)
    {
        std::vector<Line> lines;
        lines.push_back(NumberLine("Crosshead speed", r.crossheadSpeed, r.units.speed, 2));
        lines.push_back(NumberLine("Jog speed", r.jogSpeed, r.units.speed, 2));
        lines.push_back(NumberLine("Load error correction", r.loadErrorPercent, "%", 2));
        lines.push_back(TextLine("Test type", TestKindText(r.kind)));
        // Older files have no sampling rate (0)
        if (r.samplingRateHz > 0.0)
            lines.push_back(NumberLine("Sampling rate", r.samplingRateHz, "Hz", 0));
        else
            lines.push_back(TextLine("Sampling rate", "-", "Hz"));
        lines.push_back(NumberLine("Samples", (double)r.data.size(), "", 0));
        if (!r.appVersion.empty())
            lines.push_back(TextLine("Exported with", "LR 10K software " + r.appVersion));
        if (!r.exportedAt.empty())
            lines.push_back(TextLine("Exported", r.exportedAt));
        return lines;
    }

    // -------------------------------------------------------------------------
    // Graph sheets
    // -------------------------------------------------------------------------

    enum class Sheet { LoadExtension, EngineeringStress, TrueStress };

    struct Row
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct PointRow
    {
        int id = -1;  //!< TestCalc::PointId (0..6)
        Row at;
    };

    //! The two tables on a graph sheet: the curve (columns D/E) and the crucial points (columns A..C).
    struct SheetRows
    {
        std::vector<Row> curve;
        std::vector<PointRow> points;
    };

    constexpr int kPointIdCount = 7; //!< IDs 0..6 have a name and a chart label

    /**
        @brief  The rows of one graph sheet.
        @details Load-extension: extension minus the zero offset, load.
                 Engineering stress: strain in percent, stress. True stress: true strain in percent, true stress.
                 Crucial points (load-extension and engineering stress sheets only), in the same units as the curve:
                 first the points that sit on a recorded sample (in curve order), then any others (in the order
                 placed). Points with an unknown ID are left out.
    **/
    inline SheetRows BuildSheet(const Report& r, Sheet sheet)
    {
        const double a0 = AreaForExport(r);
        const double l0 = r.specimen.SafeLength();

        auto convert = [&](double extension, double load) {
            const double x = extension - r.zeroOffset;
            if (sheet == Sheet::LoadExtension)
                return Row{ x, load };
            const double strain = MaterialCalc::EngineeringStrain(x, l0);
            const double stress = MaterialCalc::EngineeringStress(load, a0);
            if (sheet == Sheet::EngineeringStress)
                return Row{ strain * 100.0, stress };
            return Row{ MaterialCalc::TrueStrain(strain) * 100.0, MaterialCalc::TrueStress(stress, strain) };
        };

        SheetRows rows;
        rows.curve.reserve(r.data.size());
        for (const TestCalc::Sample& s : r.data)
            rows.curve.push_back(convert(s.extension, s.load));

        if (sheet == Sheet::TrueStress)
            return rows;

        // Crucial points: snapped points have exactly the x of a sample
        std::vector<bool> written(r.points.size(), false);
        for (const TestCalc::Sample& s : r.data)
        {
            for (size_t i = 0; i < r.points.size(); ++i)
            {
                const TestCalc::MarkedPoint& p = r.points[i];
                if (written[i] || p.id < 0 || p.id >= kPointIdCount || p.at.extension != s.extension)
                    continue;
                rows.points.push_back({ p.id, convert(p.at.extension, p.at.load) });
                written[i] = true;
            }
        }
        for (size_t i = 0; i < r.points.size(); ++i)
        {
            const TestCalc::MarkedPoint& p = r.points[i];
            if (!written[i] && p.id >= 0 && p.id < kPointIdCount)
                rows.points.push_back({ p.id, convert(p.at.extension, p.at.load) });
        }
        return rows;
    }
}
