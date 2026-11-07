/**

    @file      LR10KMeasurement.h
    @brief     Turns the machine's raw readings into load (N) and extension (mm), with the zero point.
    @details   Portable (no Windows or GUI code) and unit tested (tests/measurement_test.cpp).
               Thread safe: used by the GUI thread and the interpreter thread.

               Zero point:
               - Load: the next data package after RequestLoadZero() (and after connecting) becomes the
                 zero. Until then the load is 0. This replaces the old "offset 0 = take the next reading"
                 rule, which re-zeroed on every reading if the zero reading happened to be exactly 0.
               - Extension: the machine's distance sum (LR10KReadings::distance), which LR10KMachine::
                 ResetDistance() restarts at 0.

**/
#pragma once

#include "LR10KMachine.h"

#include <cstdint>
#include <mutex>

class LR10KMeasurement
{
public:
    //! Conversion from machine counts to physical units.
    struct Calibration
    {
        double newtonPerCount = 0.0; //!< e.g. LR10KAppGlobals::Convert_to_Newton
        double mmPerCount = 0.0;     //!< e.g. LR10KAppGlobals::Convert_to_mm
    };

    //! Load and extension in base units (before tension/compression sign and display units).
    struct Sample
    {
        double loadNewton = 0.0;
        double extensionMm = 0.0;
        bool loadZeroPending = false; //!< Waiting for the next data package to zero the load (load is 0 meanwhile).
    };

    explicit LR10KMeasurement(Calibration calibration) : m_calibration(calibration) {}

    /**
        @brief  Zero the load at the next data package after @p now.
        @param  now - The machine's current readings (only the package count is used).
    **/
    void RequestLoadZero(const LR10KReadings& now)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_zeroPending = true;
        m_zeroAfterPackage = now.dataPackages;
    }

    //! Forget the zero point: the first data package with a count above 0 becomes the zero. (For a fresh machine
    //! whose package count starts at 0; the app uses RequestLoadZero() on disconnect, since its machine keeps counting.)
    void Reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_zeroPending = true;
        m_zeroAfterPackage = 0;
        m_loadOffset = 0;
    }

    //! The current load and extension from the machine's readings (applies a pending load zero).
    Sample FromReadings(const LR10KReadings& readings)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_zeroPending && readings.dataPackages > m_zeroAfterPackage)
        {
            m_loadOffset = readings.load;
            m_zeroPending = false;
        }

        Sample sample;
        sample.loadZeroPending = m_zeroPending;
        sample.loadNewton = m_zeroPending ? 0.0 : ((double)readings.load - (double)m_loadOffset) * m_calibration.newtonPerCount;
        sample.extensionMm = (double)readings.distance * m_calibration.mmPerCount;
        return sample;
    }

private:
    const Calibration m_calibration;

    std::mutex m_mutex;
    bool m_zeroPending = true;              //!< True until the first package after a zero request / connecting
    std::uint64_t m_zeroAfterPackage = 0;   //!< The package count when zero was requested
    std::int16_t m_loadOffset = 0;          //!< Raw load at the zero point (counts)
};
