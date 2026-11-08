/**

    @file      ExtensometerFilter.h
    @brief     Smoothing of the optical extensometer's marker distance (pixels), and the strain from it.
    @details   Portable (no OpenCV or GUI code) and unit tested (tests/extensometer_filter_test.cpp).

               The old filter was an exponential average with a fixed weight per camera frame (0.03). At
               ~25 frames/s that is a time constant of about 1.3 s, so the optical strain lagged the load by
               more than a second: the stress-strain curve leaned over and the E-modulus came out too high.
               And the lag changed with the camera's frame rate (and with recording, which slows the loop).
               This filter uses a time constant in seconds, the same at any frame rate.

               The dead-band: changes smaller than deadbandPx between the raw and the smoothed distance are
               ignored, so the value does not wander while nothing moves. It was 0.15 px; since the marker centres
               are measured to a fraction of a pixel (ExtensometerCentroid.h) the noise is several times lower, and
               0.15 px hid a real part of the elastic region (with L0 = 400 px, 0.2 % strain is only 0.8 px). Now
               0.05 px (kDefaultDeadbandPx), adjustable in the extensometer window. NoiseMeter shows the measured
               noise, to compare with it.

**/
#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

class ExtensometerFilter
{
public:
    static constexpr double kDefaultDeadbandPx = 0.05; //!< Was 0.15 px before the sub-pixel marker centres
    static constexpr double kMaxDeadbandPx = 0.5;      //!< Upper end of the slider in the extensometer window

    explicit ExtensometerFilter(double timeConstantS = 0.25, double deadbandPx = kDefaultDeadbandPx)
        : m_timeConstantS(timeConstantS), m_deadbandPx(deadbandPx) {}

    void SetTimeConstant(double seconds) { m_timeConstantS = seconds > 0.0 ? seconds : 0.0; }
    double TimeConstant() const { return m_timeConstantS; }

    //! 0 .. kMaxDeadbandPx; 0 turns the dead-band off.
    void SetDeadband(double px) { m_deadbandPx = px < 0.0 ? 0.0 : (px > kMaxDeadbandPx ? kMaxDeadbandPx : px); }
    double Deadband() const { return m_deadbandPx; }

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

/**
    @brief  The pixel noise of the raw marker distance, to compare with the dead-band.
    @details The standard deviation of the frame-to-frame changes over the last `window` frames, divided by
             sqrt(2) (the change between two frames holds the noise of both). The mean change is subtracted, so a
             steady stretch (a test at constant speed) does not count as noise; only shaking and pixel noise do.
**/
class NoiseMeter
{
public:
    explicit NoiseMeter(std::size_t window = 60) : m_window(window < 3 ? 3 : window) {}

    void Reset() { m_diffs.clear(); m_next = 0; m_hasLast = false; }

    void Add(double distancePx)
    {
        if (m_hasLast)
        {
            const double d = distancePx - m_last;
            if (m_diffs.size() < m_window) m_diffs.push_back(d);
            else m_diffs[m_next] = d;
            m_next = (m_next + 1) % m_window;
        }
        m_last = distancePx;
        m_hasLast = true;
    }

    //! Enough frames for a meaningful value (half the window).
    bool Ready() const { return m_diffs.size() >= m_window / 2; }

    //! Noise of one distance measurement in pixels (1 standard deviation); 0 until Ready().
    double NoisePx() const
    {
        if (!Ready()) return 0.0;
        double mean = 0.0;
        for (double d : m_diffs) mean += d;
        mean /= (double)m_diffs.size();
        double sq = 0.0;
        for (double d : m_diffs) sq += (d - mean) * (d - mean);
        return std::sqrt(sq / (double)(m_diffs.size() - 1)) / std::sqrt(2.0);
    }

private:
    std::size_t m_window;
    std::vector<double> m_diffs;
    std::size_t m_next = 0;
    double m_last = 0.0;
    bool m_hasLast = false;
};
