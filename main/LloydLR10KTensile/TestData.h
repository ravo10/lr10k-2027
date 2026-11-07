/**

    @file      TestData.h
    @brief     Operations on a test's curve and crucial points: smoothing, snapping, the maximum, the list of points.
    @details   Portable (no ImGui or Windows code) and unit tested (tests/test_data_test.cpp).

               The functions are templates, so they work directly on the GUI's types (ImVector<ImVec2>,
               std::vector<std::tuple<ImPlotPoint, int>>) without copying, and on plain types in the tests:
               - a curve is anything with size() and operator[] whose elements have .x (extension) and .y (load);
               - a point list is a std::vector of tuples or pairs: std::get<0> is the point, std::get<1> its ID
                 (TestCalc::PointId: 0 proportional limit, 1 yield, 2 UTS, 3 fracture, 5/6 E-modulus bounds).

               Moved out of Calculator.cpp and TabSystem.cpp. Fixed on the way:
               - the moving average divided by the window size, but adds window/2*2+1 values: with an even
                 window (the "Smoothing Level" slider allows 2..50) every smoothed load was too high, by 50 %
                 at 2 and 25 % at 4. The graph draws the smoothed curve and the points snap to it, so the UTS
                 and the Excel values were too high too.

**/
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace TestData
{
    template <class Curve>
    int Count(const Curve& curve) { return (int)curve.size(); }

    //! Index of the curve point whose extension (x) is closest to `x`; -1 for an empty curve.
    template <class Curve>
    int ClosestIndexByX(const Curve& curve, double x)
    {
        const int n = Count(curve);
        if (n == 0)
            return -1;
        int best = 0;
        double bestDistance = std::numeric_limits<double>::max();
        for (int i = 0; i < n; ++i)
        {
            const double distance = std::fabs((double)curve[i].x - x);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = i;
            }
        }
        return best;
    }

    /**
        @brief  Moving average of the load (y) over `window` points, centred; the extension (x) is kept.
        @details The first and last window/2 points are not changed (no full window around them).
                 A window below 2, or longer than the curve, returns the curve unchanged.
    **/
    template <class Curve>
    Curve MovingAverageY(const Curve& curve, int window)
    {
        const int n = Count(curve);
        if (window < 2 || n < window)
            return curve;

        Curve smoothed = curve;
        const int half = window / 2;
        const int values = 2 * half + 1; // The points actually added up (was divided by `window`)
        for (int i = half; i < n - half; ++i)
        {
            double sum = 0.0;
            for (int j = -half; j <= half; ++j)
                sum += (double)curve[i + j].y;
            smoothed[i].y = static_cast<decltype(smoothed[i].y)>(sum / values);
        }
        return smoothed;
    }

    //! Moves each point onto the curve point with the closest extension (both x and y are taken from the curve).
    template <class Points, class Curve>
    void SnapToCurve(Points& points, const Curve& curve)
    {
        for (auto& item : points)
        {
            auto& point = std::get<0>(item);
            const int index = ClosestIndexByX(curve, (double)point.x);
            if (index < 0)
                return;
            point.x = curve[index].x;
            point.y = curve[index].y;
        }
    }

    //! Index of the highest load (the UTS); -1 if the curve is empty or no load is above 0.
    template <class Curve>
    int MaxLoadIndex(const Curve& curve)
    {
        int best = -1;
        double bestLoad = 0.0;
        for (int i = 0; i < Count(curve); ++i)
        {
            if ((double)curve[i].y > bestLoad)
            {
                bestLoad = (double)curve[i].y;
                best = i;
            }
        }
        return best;
    }

    /**
        @brief  Whether a new reading is added to the test's curve.
        @details No negative values, and the extension must grow (a shaking rig must not draw back and forth).
                 lastX == 0 counts as "no point yet" (as before), so the first reading is always taken.
    **/
    inline bool AcceptsNewPoint(double lastX, double x, double y)
    {
        return x >= 0.0 && y >= 0.0 && (lastX == 0.0 || x > lastX);
    }

    //! What to do at the travel limit ("Stop after a distance" in the test settings).
    enum class TravelAction { None, EndTest, PauseTest };

    /**
        @param  limit         - Distance in the test's unit; 0 or less: no limit.
        @param  extension     - Current extension.
        @param  actionSetting - 0: end the test, 1: pause it (the setting's index).
    **/
    inline TravelAction TravelLimitAction(double limit, double extension, int actionSetting)
    {
        if (!(limit > 0.0) || extension < limit)
            return TravelAction::None;
        switch (actionSetting)
        {
        case 0: return TravelAction::EndTest;
        case 1: return TravelAction::PauseTest;
        default: return TravelAction::None;
        }
    }

    // -------------------------------------------------------------------------
    // The list of crucial points
    // -------------------------------------------------------------------------

    template <class Points>
    bool HasId(const Points& points, int id)
    {
        return std::any_of(points.begin(), points.end(), [id](const auto& item) { return std::get<1>(item) == id; });
    }

    //! Pointer to the first point with this ID, or nullptr.
    template <class Points>
    const auto* FindId(const Points& points, int id)
    {
        for (const auto& item : points)
            if (std::get<1>(item) == id)
                return &std::get<0>(item);
        return static_cast<decltype(&std::get<0>(points.front()))>(nullptr);
    }

    //! Removes every point with this ID; returns how many were removed.
    template <class Points>
    std::size_t RemoveId(Points& points, int id)
    {
        const std::size_t before = points.size();
        points.erase(std::remove_if(points.begin(), points.end(), [id](const auto& item) { return std::get<1>(item) == id; }), points.end());
        return before - points.size();
    }

    //! The first point of each ID, in the order they were added (later duplicates are left out).
    template <class Points>
    Points FirstOfEachId(const Points& points)
    {
        Points first;
        for (const auto& item : points)
            if (!HasId(first, std::get<1>(item)))
                first.push_back(item);
        return first;
    }

    //! Where the two E-modulus bounds start (indices into the curve), if there is enough data: points 10 and 50.
    inline bool DefaultEModulusBounds(int curvePoints, int& lower, int& upper)
    {
        if (curvePoints <= 50)
            return false;
        lower = 10;
        upper = 50;
        return true;
    }
}
