/**

    @file      MaterialCalc.h
    @brief     Material property formulas (stress, strain, Young's modulus, 3-point bending).
    @details   Plain functions with no GUI, Windows or machine dependencies, so they can be
               unit tested on any platform (tests/material_calc_test.cpp) and later ported as-is.

               Units: every function works in whatever consistent unit system the caller uses.
               With the SI setting that is N, mm, N/mm^2 (= MPa). With the imperial setting it
               is lbf, in, lbf/in^2 (= psi). Strains are dimensionless (not percent).

**/
#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace MaterialCalc
{
    //! \brief Shape of the specimen cross-section (same numbers as "areaOptionPicked" in the GUI).
    enum CrossSection : int { Rectangular = 0, Circular = 1 };

    /**
        @brief  Original cross-sectional area A_0.
        @param  shape     - Rectangular (thickness * width) or Circular (pi * d^2 / 4).
        @retval The area, or 0 if the dimensions are missing/invalid.
    **/
    inline double CrossSectionArea(int shape, double thickness, double width, double diameter)
    {
        if (shape == Rectangular)
            return (thickness > 0.0 && width > 0.0) ? thickness * width : 0.0;
        if (shape == Circular)
            return diameter > 0.0 ? 3.14159265358979323846 * diameter * diameter / 4.0 : 0.0;
        return 0.0;
    }

    //! \brief Engineering (nominal) stress: sigma_e = F / A_0.
    inline double EngineeringStress(double force, double area0) { return force / area0; }

    //! \brief Engineering (nominal) strain: epsilon_e = delta_L / L_0.
    inline double EngineeringStrain(double deltaLength, double length0) { return deltaLength / length0; }

    /**
        @brief  Clamp a stress or strain to zero or above.
        @details Tension and compression are chosen in the GUI, which already flips the sign so the
                 test direction is positive. Negative values are only noise/slack around the zero
                 point (e.g. before the specimen is loaded) and are shown as 0. NaN stays NaN.
    **/
    inline double NonNegative(double value) { return value < 0.0 ? 0.0 : value; }

    /**
        @brief  True stress: sigma_t = sigma_e * (1 + epsilon_e).
        @note   Assumes constant volume and uniform deformation: only valid up to the UTS
                (before necking). Values after the UTS (e.g. at fracture) are not physical.
    **/
    inline double TrueStress(double engineeringStress, double engineeringStrain) { return engineeringStress * (1.0 + engineeringStrain); }

    //! \brief True (logarithmic) strain: epsilon_t = ln(1 + epsilon_e). Same validity as TrueStress().
    inline double TrueStrain(double engineeringStrain) { return std::log(1.0 + engineeringStrain); }

    //! \brief Result of a least-squares straight line fit.
    struct LinearFit
    {
        double slope{ 0.0 };
        double intercept{ 0.0 };
        int    points{ 0 };     //!< Number of points inside the window that were used.
        bool   valid{ false };  //!< False if fewer than 2 points, or all x values were the same.
    };

    /**
        @brief  Least-squares fit y = slope * x + intercept, using only points with lowerX <= x <= upperX.
        @details Used for Young's modulus (stress vs. strain) and the bending stiffness (load vs. deflection).
                 The window limits may be given in either order.
    **/
    inline LinearFit FitLine(const std::vector<double>& x, const std::vector<double>& y, double lowerX, double upperX)
    {
        const double lo = lowerX < upperX ? lowerX : upperX;
        const double hi = lowerX < upperX ? upperX : lowerX;
        const std::size_t count = x.size() < y.size() ? x.size() : y.size();

        // Two passes around the mean: numerically stable even for tiny strains (e.g. 0.0005..0.0025)
        double sumX = 0.0, sumY = 0.0;
        int n = 0;
        for (std::size_t i = 0; i < count; i++)
        {
            if (x[i] >= lo && x[i] <= hi && std::isfinite(x[i]) && std::isfinite(y[i]))
            {
                sumX += x[i];
                sumY += y[i];
                n++;
            }
        }

        LinearFit fit;
        fit.points = n;
        if (n < 2)
            return fit;

        const double meanX = sumX / n;
        const double meanY = sumY / n;
        double sxx = 0.0, sxy = 0.0;
        for (std::size_t i = 0; i < count; i++)
        {
            if (x[i] >= lo && x[i] <= hi && std::isfinite(x[i]) && std::isfinite(y[i]))
            {
                sxx += (x[i] - meanX) * (x[i] - meanX);
                sxy += (x[i] - meanX) * (y[i] - meanY);
            }
        }
        if (sxx <= 0.0)
            return fit; // All x the same: no slope

        fit.slope = sxy / sxx;
        fit.intercept = meanY - fit.slope * meanX;
        fit.valid = true;
        return fit;
    }

    // -------------------------------------------------------------------------
    // 3-point bending (support span L, width w, thickness/height t)
    // -------------------------------------------------------------------------

    //! \brief Maximum bending moment at mid-span: M = F * L / 4.
    inline double BendingMoment3Point(double force, double span) { return force * span / 4.0; }

    //! \brief Flexural stress at the outer fibre: sigma_f = 3 * F * L / (2 * w * t^2).
    inline double FlexuralStress3Point(double force, double span, double width, double thickness)
    {
        return 3.0 * force * span / (2.0 * width * thickness * thickness);
    }

    /**
        @brief  Flexural modulus from the slope of the load-deflection line: E_f = m * L^3 / (4 * w * t^3).
        @param  slope - Load per deflection (e.g. N/mm) in the linear part of the curve.
    **/
    inline double FlexuralModulus3Point(double slope, double span, double width, double thickness)
    {
        return slope * span * span * span / (4.0 * width * thickness * thickness * thickness);
    }
}
