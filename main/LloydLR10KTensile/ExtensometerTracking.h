/**

    @file      ExtensometerTracking.h
    @brief     Picks the two markers from the detected blobs and rejects single bad frames.
    @details   Portable (no OpenCV or GUI code) and unit tested (tests/extensometer_tracking_test.cpp).

               Under bad lighting the colour mask is less clean:
               - a reflection or a part of the clamp shows up as a third blob: the frame counted as "lost"
                 (and, during a test, could pause it), although the two markers were still there;
               - a half-lit marker gives a shifted centre for a frame or two: the distance jumped.
               So:
               - with more than two blobs, the two closest to the markers' last positions are used (if they
                 moved less than maxMovePx since then);
               - a distance that differs from the smoothed one by more than maxStepFraction of L0 in one frame
                 is ignored, unless it stays like that for confirmFrames frames in a row (a real fast change,
                 e.g. at fracture).

**/
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace ExtensometerTracking
{
    struct Point
    {
        double x = 0.0;
        double y = 0.0;
    };

    inline double Distance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y); }

    //! The two markers, upper first (smaller y). found == false if they could not be told apart.
    struct Markers
    {
        bool found = false;
        Point upper;
        Point lower;
    };

    /**
        @brief  The two markers among the detected blob centres.
        @param  centers   - Blob centres in this frame.
        @param  previous  - The markers in the last good frame (previous.found == false: none yet).
        @param  maxMovePx - How far a marker may move between good frames to be followed through extra blobs.
    **/
    inline Markers PickMarkers(const std::vector<Point>& centers, const Markers& previous, double maxMovePx = 25.0)
    {
        Markers result;
        if (centers.size() == 2)
        {
            result.found = true;
            result.upper = centers[0];
            result.lower = centers[1];
            if (result.lower.y < result.upper.y)
                std::swap(result.upper, result.lower);
            return result;
        }
        if (centers.size() < 2 || !previous.found)
            return result; // 0 or 1 blob, or extra blobs without a previous position to compare with

        // More than two blobs: follow the markers from their last positions
        auto nearest = [&](Point target, int skip) {
            int best = -1;
            double bestDistance = maxMovePx;
            for (int i = 0; i < (int)centers.size(); ++i)
            {
                if (i == skip) continue;
                const double d = Distance(centers[i], target);
                if (d <= bestDistance) { bestDistance = d; best = i; }
            }
            return best;
        };
        const int upper = nearest(previous.upper, -1);
        if (upper < 0) return result;
        const int lower = nearest(previous.lower, upper);
        if (lower < 0) return result;

        result.found = true;
        result.upper = centers[upper];
        result.lower = centers[lower];
        if (result.lower.y < result.upper.y)
            std::swap(result.upper, result.lower);
        return result;
    }

    //! Ignores single-frame jumps of the marker distance.
    class JumpGate
    {
    public:
        explicit JumpGate(double maxStepFraction = 0.02, int confirmFrames = 4)
            : m_maxStepFraction(maxStepFraction), m_confirmFrames(confirmFrames) {}

        /**
            @brief  True if this distance can be used.
            @param  distancePx  - Measured distance in this frame.
            @param  referencePx - The current (smoothed) distance.
            @param  l0Px        - The gauge length in pixels (the step limit is a fraction of it).
        **/
        bool Accept(double distancePx, double referencePx, double l0Px)
        {
            const double limit = m_maxStepFraction * (l0Px > 0.0 ? l0Px : referencePx);
            if (std::fabs(distancePx - referencePx) <= limit)
            {
                m_rejected = 0;
                return true;
            }
            // A big step: only believe it once it stays (fracture, a fast real change)
            if (++m_rejected >= m_confirmFrames)
            {
                m_rejected = 0;
                return true;
            }
            return false;
        }

        void Reset() { m_rejected = 0; }
        int RejectedInARow() const { return m_rejected; }

    private:
        double m_maxStepFraction;
        int m_confirmFrames;
        int m_rejected = 0;
    };
}
