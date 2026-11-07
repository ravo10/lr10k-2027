/**

    @file      ExcelReport.h
    @brief     Everything the Excel export writes, as plain data, and the numbers for each sheet.
    @details   Portable (no GUI, Windows or libxlsxwriter code) and unit tested (tests/excel_report_test.cpp).
               ExcelExport.cpp fills a Report from a test tab and only does the formatting/writing.

**/
#pragma once

#include "TestCalc.h"

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

    //! One finished test: all the data the export needs.
    struct Report
    {
        std::string date;
        std::string description;
        Units units;

        bool tension = true;
        TestCalc::Specimen specimen;
        double storedArea = 0.0;  //!< The area saved with the test (only used if the dimensions give none)
        double zeroOffset = 0.0;  //!< The test's zero point on the extension axis (slack)

        // Results calculated in the app (TestCalc)
        double youngsModulus = 0.0;  //!< In units.modulus
        TestCalc::StressStrain nominal[TestCalc::TensileResults::kPoints]; //!< Prop. limit, yield, UTS, fracture

        // Machine settings
        double crossheadSpeed = 0.0;
        double jogSpeed = 0.0;
        double loadErrorPercent = 0.0;
        double samplingRateHz = 0.0;

        std::vector<TestCalc::Sample> data;          //!< Extension/load as recorded (before the zero offset)
        std::vector<TestCalc::MarkedPoint> points;   //!< Crucial points as placed on the curve (before the zero offset)
    };

    //! The report and stress sheets need the specimen dimensions.
    inline bool HasStressSheets(const Report& r) { return TestCalc::HasDimensions(r.specimen); }

    //! A_0 for the export: from the dimensions (older files have a wrong stored area for circular specimens),
    //! else the stored area, else 1.
    inline double AreaForExport(const Report& r)
    {
        const double a = MaterialCalc::CrossSectionArea(r.specimen.crossSection, r.specimen.thickness, r.specimen.width, r.specimen.diameter);
        if (a > 0.0) return a;
        return r.storedArea > 0.0 ? r.storedArea : 1.0;
    }

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
