/**

    @file      AutoPoints.h
    @brief     Automatic placement of the E-modulus limits, the yield strength (Rp0.2) and the proportional limit.
    @details   Portable (no ImGui code) and unit tested (tests/auto_points_test.cpp), on curves made with the
               Ramberg-Osgood formula, whose Rp0.2 is known exactly.

               Works on the load-extension curve (any units), from the start to the maximum load:
               1. The elastic line: the steepest straight part between 10 % and 50 % of the maximum load. Windows
                  of 20 % of the maximum load are fitted with a line (least squares); of those that are straight
                  (R^2 >= 0.995), the steepest is taken. This skips the soft start of a test (slack, the grips
                  settling), where the old fixed limits (sample 10 and 50, or 0.05 % .. 0.25 % strain) often were.
                  The two E-modulus limits are put at the ends of that window.
               2. Rp0.2: the elastic line moved by 0.2 % strain (0.002 * L0 in extension) to the right; the yield
                  point is where the curve falls below it. The line starts where the fitted line reaches zero
                  load, not at extension 0, so the slack at the start is not counted as strain (as the tensile
                  test standards do).
               3. The proportional limit: the same with 0.01 % (Rp0.01, the "technical" proportional limit).

               For steel with an upper and lower yield point (a yield plateau), Rp0.2 lands on the plateau.
               Upper/lower yield (ReH/ReL) are not placed (they would need their own point types).

               The E-modulus from the crosshead travel stays lower than the material's (the machine and the
               grips stretch too); only the optical extensometer measures the specimen alone.

**/
#pragma once

#include <cmath>
#include <vector>

namespace AutoPoints
{
    //! A least-squares line load = slope * extension + intercept, fitted on curve[first..last].
    struct Line
    {
        bool ok = false;
        double slope = 0.0;
        double intercept = 0.0;
        double r2 = 0.0;
        int first = -1;
        int last = -1;

        //! The extension where the line reaches zero load (the start of the elastic part, without the slack).
        double ExtensionAtZeroLoad() const { return slope != 0.0 ? -intercept / slope : 0.0; }
        double LoadAt(double extension) const { return slope * extension + intercept; }
    };

    struct Options
    {
        double searchFrom = 0.10;     //!< Elastic windows start at this fraction of the maximum load ...
        double searchTo = 0.50;       //!< ... and end at most here
        double windowSpan = 0.20;     //!< A window covers this fraction of the maximum load
        double windowStep = 0.02;     //!< Windows move by this fraction
        double minR2 = 0.995;         //!< A window this straight (or straighter) counts as straight
        int minPoints = 8;            //!< Fewer samples in a window: not used
        double yieldOffsetStrain = 0.002;         //!< Rp0.2
        double proportionalOffsetStrain = 0.0001; //!< Rp0.01
    };

    template <class Curve>
    int Size(const Curve& curve) { return (int)curve.size(); }

    template <class Curve>
    Line FitRange(const Curve& curve, int first, int last)
    {
        Line line;
        line.first = first;
        line.last = last;
        const int n = last - first + 1;
        if (first < 0 || last >= Size(curve) || n < 2)
            return line;

        double sx = 0, sy = 0;
        for (int i = first; i <= last; ++i) { sx += curve[i].x; sy += curve[i].y; }
        const double mx = sx / n, my = sy / n;
        double sxx = 0, sxy = 0, syy = 0;
        for (int i = first; i <= last; ++i)
        {
            const double dx = curve[i].x - mx, dy = curve[i].y - my;
            sxx += dx * dx; sxy += dx * dy; syy += dy * dy;
        }
        if (sxx <= 0.0 || syy <= 0.0)
            return line;
        line.slope = sxy / sxx;
        line.intercept = my - line.slope * mx;
        line.r2 = (sxy * sxy) / (sxx * syy);
        line.ok = line.slope > 0.0;
        return line;
    }

    //! Index of the highest load, or -1.
    template <class Curve>
    int MaxLoadIndex(const Curve& curve)
    {
        int best = -1;
        double bestLoad = 0.0;
        for (int i = 0; i < Size(curve); ++i)
            if (curve[i].y > bestLoad) { bestLoad = curve[i].y; best = i; }
        return best;
    }

    //! The elastic line (see the file description). line.ok == false if the curve has too little data.
    template <class Curve>
    Line FindElasticLine(const Curve& curve, const Options& options = {})
    {
        const int top = MaxLoadIndex(curve);
        if (top < options.minPoints)
            return {};
        const double maxLoad = curve[top].y;

        // First index (up to the maximum) where the load reaches `fraction` of the maximum
        auto firstAtOrAbove = [&](double fraction, int from) {
            for (int i = from; i <= top; ++i)
                if (curve[i].y >= fraction * maxLoad)
                    return i;
            return -1;
        };

        Line steepest, straightest;
        for (double from = options.searchFrom; from + options.windowSpan <= options.searchTo + 1e-9; from += options.windowStep)
        {
            const int a = firstAtOrAbove(from, 0);
            const int b = a < 0 ? -1 : firstAtOrAbove(from + options.windowSpan, a);
            if (a < 0 || b < 0 || b - a + 1 < options.minPoints)
                continue;
            const Line line = FitRange(curve, a, b);
            if (!line.ok)
                continue;
            if (line.r2 >= options.minR2 && (!steepest.ok || line.slope > steepest.slope))
                steepest = line;
            if (!straightest.ok || line.r2 > straightest.r2)
                straightest = line;
        }
        return steepest.ok ? steepest : straightest; // No window straight enough (a very curved test): the straightest
    }

    //! A point on the curve (interpolated between two samples).
    struct Point
    {
        bool found = false;
        double x = 0.0;
        double y = 0.0;
    };

    /**
        @brief  Where the curve falls below the elastic line moved right by `offsetExtension` (Rp0.2: 0.002 * L0).
        @details Searched from the elastic window to the maximum load. Not found if the curve never falls below it
                 before the maximum (a brittle test that breaks while still straight).
    **/
    template <class Curve>
    Point OffsetIntersection(const Curve& curve, const Line& line, double offsetExtension)
    {
        Point point;
        const int top = MaxLoadIndex(curve);
        if (!line.ok || top < 0 || !(offsetExtension > 0.0))
            return point;
        const double start = line.ExtensionAtZeroLoad() + offsetExtension;
        auto above = [&](int i) { return curve[i].y - line.slope * (curve[i].x - start); }; // > 0: curve above the moved line

        for (int i = line.first + 1; i <= top; ++i)
        {
            const double before = above(i - 1), now = above(i);
            if (before > 0.0 && now <= 0.0)
            {
                const double t = before / (before - now);
                point.found = true;
                point.x = curve[i - 1].x + t * (curve[i].x - curve[i - 1].x);
                point.y = curve[i - 1].y + t * (curve[i].y - curve[i - 1].y);
                return point;
            }
        }
        return point;
    }

    //! Everything Place() found.
    struct Placement
    {
        Line elastic;                 //!< elastic.first / elastic.last: where the E-modulus limits go
        Point yield;                  //!< Rp0.2
        Point proportionalLimit;      //!< Rp0.01
    };

    /**
        @param  gaugeLength - L0 in the curve's length unit; 0 (unknown): only the E-modulus limits are found.
        @param  findYield   - false for 3-point bending (an offset yield does not apply there).
    **/
    template <class Curve>
    Placement Place(const Curve& curve, double gaugeLength, bool findYield = true, const Options& options = {})
    {
        Placement p;
        p.elastic = FindElasticLine(curve, options);
        if (!p.elastic.ok || !findYield || !(gaugeLength > 0.0))
            return p;
        p.yield = OffsetIntersection(curve, p.elastic, options.yieldOffsetStrain * gaugeLength);
        p.proportionalLimit = OffsetIntersection(curve, p.elastic, options.proportionalOffsetStrain * gaugeLength);
        return p;
    }
}
