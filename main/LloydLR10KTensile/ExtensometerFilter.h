/**

    @file      ExtensometerFilter.h
    @brief     Smoothing of the optical extensometer's marker distance (pixels), and the strain from it.
    @details   Portable (no OpenCV or GUI code) and unit tested (tests/extensometer_filter_test.cpp).

               The old filter was an exponential average with a fixed weight per camera frame (0.03). At
               ~25 frames/s that is a time constant of about 1.3 s, so the optical strain lagged the load by
               more than a second: the stress-strain curve leaned over and the E-modulus came out too high.
               And the lag changed with the camera's frame rate (and with recording, which slows the loop).
               This filter uses a time constant in seconds, the same at any frame rate.

               The dead-band is kept: changes smaller than deadbandPx between the raw and the smoothed distance
               are ignored, so the value does not wander while nothing moves.

**/
#pragma once

#include <cmath>

class ExtensometerFilter
{
public:
    explicit ExtensometerFilter(double timeConstantS = 0.25, double deadbandPx = 0.15)
        : m_timeConstantS(timeConstantS), m_deadbandPx(deadbandPx) {}

    void SetTimeConstant(double seconds) { m_timeConstantS = seconds > 0.0 ? seconds : 0.0; }
    double TimeConstant() const { return m_timeConstantS; }

    //! Start again from this distance (the gauge length L0 in pixels, "SET L0 / RESET").
    void Reset(double distancePx)
    {
        m_initialPx = distancePx;
        m_smoothPx = distancePx;
        m_initialized = distancePx > 0.0;
    }

    bool Initialized() const { return m_initialized; }
    double InitialPx() const { return m_initialPx; }
    double SmoothPx() const { return m_smoothPx; }

    /**
        @brief  Add a measured distance.
        @param  distancePx - Distance between the two markers in this frame.
        @param  dtS        - Time since the previous frame (seconds).
    **/
    void Update(double distancePx, double dtS)
    {
        if (!m_initialized)
        {
            Reset(distancePx);
            return;
        }
        if (std::fabs(distancePx - m_smoothPx) <= m_deadbandPx)
            return;
        if (m_timeConstantS <= 0.0 || dtS <= 0.0)
        {
            m_smoothPx = distancePx;
            return;
        }
        // Exponential smoothing with the weight for this frame's time step: 1 - e^(-dt / tau)
        const double alpha = 1.0 - std::exp(-dtS / m_timeConstantS);
        m_smoothPx += alpha * (distancePx - m_smoothPx);
    }

    //! Engineering strain (not percent) since the reset.
    double Strain() const { return m_initialized && m_initialPx > 0.0 ? (m_smoothPx - m_initialPx) / m_initialPx : 0.0; }

private:
    double m_timeConstantS;
    double m_deadbandPx;
    bool m_initialized = false;
    double m_initialPx = 0.0;
    double m_smoothPx = 0.0;
};
