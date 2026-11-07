/**

    @file      LoadSafety.h
    @brief     Software load safety: the 50 N translation blocker and the user's load limit.
    @details   Portable (no Windows or GUI code) and unit tested (tests/load_safety_test.cpp).
               Called once per frame from the GUI thread (LR10KControlSystem::LoadSafetyTick).

               Both checks were in a background thread that was no longer started after February 2026
               ("removed obsolete background threads"), so neither the 50 N blocker nor the load limit
               set under "Limits inside LR 10K" did anything. (The extension limit is sent to the LR 10K
               and enforced by the machine itself.)

               Loads are in newtons, and the sign does not matter (tension or compression).

**/
#pragma once

#include <cmath>

namespace LoadSafety
{
    constexpr double kMachineCapacityN = 10000.0;    //!< LR 10K load cell capacity
    constexpr double kTranslationBlockerN = 50.0;    //!< Default limit when moving the crosshead without a test

    //! The load limit in use: the user's limit, never above the machine capacity (and 0 or less means capacity).
    inline double EffectiveLimit(double userLimitN)
    {
        if (!(userLimitN > 0.0) || userLimitN > kMachineCapacityN)
            return kMachineCapacityN;
        return userLimitN;
    }

    //! True if the load has reached the limit (either direction).
    inline bool OverLimit(double loadN, double limitN) { return std::fabs(loadN) >= limitN; }

    /**
        @brief  Stops crosshead movement that pushes the load above a limit, but lets the user move away again.
        @details While active (the crosshead may be moving and no test runs), a stop is asked for when the load
                 is above the limit AND has risen since the lowest load seen in this movement. So:
                 - moving down onto something: 0 N -> 51 N, stop;
                 - already above the limit (e.g. a clamped specimen) and moving so the load drops: no stop
                   (the old check stopped any movement above 50 N, so the user could not move away);
                 - moving further into the load: stop again (at most once per repeatInterval).
                 The lowest load restarts when the guard becomes inactive (the machine reports "Ready") and
                 after each stop.
    **/
    class Guard
    {
    public:
        explicit Guard(double limitN = kTranslationBlockerN, double riseN = 2.0, long long repeatIntervalMs = 1000)
            : m_limitN(limitN), m_riseN(riseN), m_repeatIntervalMs(repeatIntervalMs) {}

        //! @return true if the crosshead should be stopped now.
        bool Update(bool active, double loadN, long long nowMs)
        {
            const double load = std::fabs(loadN);
            if (!active)
            {
                m_tracking = false;
                return false;
            }
            if (!m_tracking)
            {
                m_tracking = true;
                m_lowest = load;
            }
            if (load < m_lowest)
                m_lowest = load;

            const bool rising = load >= m_lowest + m_riseN;
            const bool repeatAllowed = !m_stoppedBefore || nowMs - m_lastStopMs >= m_repeatIntervalMs;
            if (load > m_limitN && rising && repeatAllowed)
            {
                m_stoppedBefore = true;
                m_lastStopMs = nowMs;
                m_lowest = load; // Only stop again if it keeps rising from here
                return true;
            }
            return false;
        }

        double Limit() const { return m_limitN; }

    private:
        double m_limitN;
        double m_riseN;
        long long m_repeatIntervalMs;

        bool m_tracking = false;
        double m_lowest = 0.0;
        bool m_stoppedBefore = false;
        long long m_lastStopMs = 0;
    };
}
