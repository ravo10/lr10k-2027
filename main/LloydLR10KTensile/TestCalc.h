/**

    @file      TestCalc.h
    @brief     The calculations for one finished test (crucial points, E-modulus, 3-point bending), on plain data.
    @details   Uses only the test's own specimen dimensions and data, never the calculator's global inputs.
               Portable (no GUI, Windows or machine code) and unit tested (tests/test_calc_test.cpp).

               Units: the test's current units (SI: N, mm, N/mm^2; imperial: lbf, in, psi), like MaterialCalc.h.
               The E-modulus is returned in N/mm^2 (MPa) or psi; the GUI shows GPa with SI units.

**/
#pragma once

#include "MaterialCalc.h"

#include <vector>

namespace TestCalc
{
    //! The specimen dimensions saved with a test.
    struct Specimen
    {
        int crossSection = MaterialCalc::Rectangular; //!< MaterialCalc::CrossSection
        double thickness = 0.0;
        double width = 0.0;
        double length = 0.0;   //!< Gauge length L_0 (tension/compression) or support span (3-point bending)
        double diameter = 0.0;

        //! A_0, or 1 if the dimensions are missing (so nothing divides by 0).
        double SafeArea() const
        {
            const double a = MaterialCalc::CrossSectionArea(crossSection, thickness, width, diameter);
            return a > 0.0 ? a : 1.0;
        }
        //! L_0, or 1 if missing.
        double SafeLength() const { return length > 0.0 ? length : 1.0; }
    };

    //! True if the dimensions needed for this cross-section and the gauge length are all above 0.
    inline bool HasDimensions(const Specimen& s)
    {
        if (s.length <= 0.0)
            return false;
        return s.crossSection == MaterialCalc::Circular ? s.diameter > 0.0 : (s.thickness > 0.0 && s.width > 0.0);
    }

    //! A new test tab's dimensions before anything is entered (0.01 everywhere).
    inline bool IsPlaceholder(const Specimen& s)
    {
        return s.thickness == 0.01 && s.width == 0.01 && s.length == 0.01 && s.diameter == 0.01;
    }

    /**
        @brief  The dimensions a test is started with.
        @details The test's own (local) values if the user entered them, otherwise the global values.
                 With "Always load from global values" on, always the global values.
                 The caller still checks HasDimensions() on the result before starting.
    **/
    inline Specimen SpecimenForNewTest(bool alwaysUseGlobal, const Specimen& local, const Specimen& global)
    {
        if (!alwaysUseGlobal && HasDimensions(local) && !IsPlaceholder(local))
            return local;
        return global;
    }

    //! One measured point: extension (x) and load (y).
    struct Sample
    {
        double extension = 0.0;
        double load = 0.0;
    };

    //! IDs of the points the user places on the curve (same numbers as in the GUI and the .lr10k files).
    enum PointId : int { ProportionalLimit = 0, Yield = 1, UltimateTensileStrength = 2, Fracture = 3, ModulusLower = 5, ModulusUpper = 6 };

    struct MarkedPoint
    {
        int id = -1;
        Sample at;
    };

    struct StressStrain
    {
        double strain = 0.0;
        double stress = 0.0;
    };

    //! Results of a tension (or plain compression) test.
    struct TensileResults
    {
        static constexpr int kPoints = 4;  //!< Proportional limit, yield, UTS, fracture (indexed by PointId)
        bool found[kPoints]{};
        StressStrain nominal[kPoints];
        StressStrain truePoint[kPoints];

        bool modulusCalculated = false;    //!< False if there is too little data (10 samples or fewer)
        double modulusLowerStrain = 0.0005; //!< The strain window of the line fit (default 0.05 % .. 0.25 %)
        double modulusUpperStrain = 0.0025;
        MaterialCalc::LinearFit modulusFit; //!< slope = E in N/mm^2 or psi (0 / invalid if too few points in the window)
    };

    /**
        @brief  Stress and strain at the marked points, and the E-modulus from the data between the modulus limits.
        @param  zeroOffset - The test's zero point on the extension axis (removes slack): strain = (x - zeroOffset) / L_0.
    **/
    inline TensileResults CalculateTensile(const Specimen& specimen, const std::vector<Sample>& data, const std::vector<MarkedPoint>& points, double zeroOffset)
    {
        TensileResults r;
        const double a0 = specimen.SafeArea();
        const double l0 = specimen.SafeLength();

        for (const MarkedPoint& p : points)
        {
            if (p.id < 0 || p.id >= TensileResults::kPoints)
                continue;
            StressStrain& nom = r.nominal[p.id];
            nom.strain = MaterialCalc::EngineeringStrain(p.at.extension - zeroOffset, l0);
            nom.stress = MaterialCalc::EngineeringStress(p.at.load, a0);
            r.truePoint[p.id].strain = MaterialCalc::TrueStrain(nom.strain);
            r.truePoint[p.id].stress = MaterialCalc::TrueStress(nom.stress, nom.strain);
            r.found[p.id] = true;
        }

        if (data.size() <= 10)
            return r;

        for (const MarkedPoint& p : points)
        {
            if (p.id == ModulusLower) r.modulusLowerStrain = MaterialCalc::EngineeringStrain(p.at.extension - zeroOffset, l0);
            if (p.id == ModulusUpper) r.modulusUpperStrain = MaterialCalc::EngineeringStrain(p.at.extension - zeroOffset, l0);
        }

        // Before the zero point (slack) the strain and the stress count as 0
        std::vector<double> strain, stress;
        strain.reserve(data.size());
        stress.reserve(data.size());
        for (const Sample& s : data)
        {
            const double extension = MaterialCalc::NonNegative(s.extension - zeroOffset);
            strain.push_back(MaterialCalc::EngineeringStrain(extension, l0));
            stress.push_back(MaterialCalc::EngineeringStress(extension == 0.0 ? 0.0 : s.load, a0));
        }

        r.modulusCalculated = true;
        r.modulusFit = MaterialCalc::FitLine(strain, stress, r.modulusLowerStrain, r.modulusUpperStrain);
        return r;
    }

    //! Results of a 3-point bending test.
    struct BendingResults
    {
        double maxLoad = 0.0;          //!< At the UTS point
        double maxTravel = 0.0;        //!< At the UTS point, from the zero point
        double bendingMoment = 0.0;    //!< M = F L / 4
        double flexuralStress = 0.0;   //!< sigma_f = 3 F L / (2 w t^2)

        bool modulusCalculated = false;
        double modulusLowerTravel = 0.0;
        double modulusUpperTravel = 0.0;
        MaterialCalc::LinearFit loadPerTravel; //!< The fitted slope of the load-deflection line
        double flexuralModulus = 0.0;          //!< E_f in N/mm^2 or psi (0 if the fit is invalid)
    };

    /**
        @brief  3-point bending results from the UTS point and the load-deflection line.
        @param  uts               - The maximum (UTS) point of the curve.
        @param  defaultLowerTravel / defaultUpperTravel - Fit window when no modulus limits are placed
                                    (the app uses 0.5 and 2.0 mm in the current length unit).
        @details Missing dimensions count as 1 (so nothing divides by 0). Rectangular cross-section only.
    **/
    inline BendingResults CalculateBending3Point(const Specimen& specimen, const std::vector<Sample>& data, const std::vector<MarkedPoint>& points,
        double zeroOffset, Sample uts, double defaultLowerTravel, double defaultUpperTravel)
    {
        const double t = specimen.thickness > 0.0 ? specimen.thickness : 1.0;
        const double w = specimen.width > 0.0 ? specimen.width : 1.0;
        const double span = specimen.SafeLength();

        BendingResults r;
        r.maxLoad = uts.load;
        r.maxTravel = uts.extension - zeroOffset;
        r.bendingMoment = MaterialCalc::BendingMoment3Point(r.maxLoad, span);
        r.flexuralStress = MaterialCalc::FlexuralStress3Point(r.maxLoad, span, w, t);

        if (data.size() <= 10)
            return r;

        // The modulus limits are read straight off the load/extension graph
        r.modulusLowerTravel = defaultLowerTravel;
        r.modulusUpperTravel = defaultUpperTravel;
        for (const MarkedPoint& p : points)
        {
            if (p.id == ModulusLower) r.modulusLowerTravel = p.at.extension;
            if (p.id == ModulusUpper) r.modulusUpperTravel = p.at.extension;
        }

        std::vector<double> travel, load;
        travel.reserve(data.size());
        load.reserve(data.size());
        for (const Sample& s : data)
        {
            travel.push_back(s.extension);
            load.push_back(s.load);
        }

        r.modulusCalculated = true;
        r.loadPerTravel = MaterialCalc::FitLine(travel, load, r.modulusLowerTravel, r.modulusUpperTravel);
        r.flexuralModulus = r.loadPerTravel.valid ? MaterialCalc::FlexuralModulus3Point(r.loadPerTravel.slope, span, w, t) : 0.0;
        return r;
    }
}
