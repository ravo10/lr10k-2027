/**

    @file      LR10KDecryption.h
    @brief     Pre-defines functions for the LR10KDecryption.cpp file.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/
#pragma once

#include <Windows.h>

#include <thread>
#include <memory> //!< Include the memory header for std::unique_ptr
#include <atomic> //!< Include the atomic header for std::atomic
#include <mutex>
#include <condition_variable>
#include <chrono>

typedef uint8_t byte;

/**
    @brief  Custom map functions - to reditect one range to another spesific range.
    @param  x       -
    @param  in_min  -
    @param  in_max  -
    @param  out_min -
    @param  out_max -
    @retval         -
**/
inline long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
    @brief  Get the current time since epoch in milliseconds.
    @retval  -
**/
inline uint64_t getCurrentTime()
{
    // Current date/time based on current system
    auto now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    return now;
}

#include "RingBuffer.h" //!< CRingBuffer (portable, unit tested in tests/ring_buffer_test.cpp)
#include "LR10KMachine.h" //!< Ring buffer + reader and interpreter threads (portable, tested in tests/machine_test.cpp)
#include "LR10KMeasurement.h" //!< Load/extension with the zero point (portable, tested in tests/measurement_test.cpp)
#include "Win32SerialPort.h" //!< The COM port
#include "SerialTrafficLog.h" //!< Optional log of all bytes sent/received (tested in tests/serial_traffic_log_test.cpp)

/**
    @namespace LR10KDataDeEncryptionSystem
    @brief     Is used to decrypt and convert to HEX data from the LR10K Tensile Machine
                using a custom decryption pattern found by some teachers at USN.
**/
namespace LR10KDataDeEncryptionSystem {
    //! \note Used for reading data from LR10K to buffer, and often includes a "m_SerialBuffer" pointer to written to as a paramter. Contains COM connection information.
    inline extern Win32SerialPort comPort{};       //!< The COM port connected to the LR 10K.
    inline extern SerialTrafficLog trafficLog{};   //!< "Log serial traffic to file" (menu). Logs nothing while not started.
    inline extern LR10KMachine machine{};          //!< The connection: ring buffer, reader thread (comPort -> buffer) and interpreter thread.
    //! \note The handle of comPort, kept for code that still uses it (INVALID_HANDLE_VALUE when closed).
    inline extern HANDLE m_hSerial = INVALID_HANDLE_VALUE;
#if UNICODE
    //! \note Show the connected COM port as a world string. E.g. COM1.
    inline extern std::wstring ConnectedCOMPortName = L"";
#else
    //! \note Show the connected COM port as a string. E.g. COM1.
    inline extern std::string ConnectedCOMPortName = "";
#endif
    //! \note Used for reading data from LR10K to buffer, and often includes a "m_SerialBuffer" pointer to written to as a paramter. Contains COM data. (a ring buffer)
    inline extern CRingBuffer& m_SerialBuffer = machine.Buffer(); //!< The machine's ring buffer (one shared instance).

    inline extern std::unique_ptr<std::thread> m_pThreadReadCom = nullptr;      //!< (INPUT) The thread that reads data from the COM port.

    inline extern std::unique_ptr<std::thread> m_pThreadConnectingToLR10K   = nullptr;   //!< The thread that contains a hooks that connects to the LR 10K Tensile Machine
    inline extern std::unique_ptr<std::thread> m_pThreadLR10KIsLoading      = nullptr;   //!< The thread that contains a hooks that setups to the LR 10K Tensile Machine

    inline extern std::atomic<bool> m_pThreadReadLR10K_stopFlag(true);          //!< Need to stop the threads "while loop" before joining it. For reading LR 10K serial buffer data.
    inline extern std::atomic<bool> m_pThreadReadCom_stopFlag(true);            //!< Need to stop the threads "while loop" before joining it. For reading COM port data.
    inline extern std::atomic<bool> m_pThreadLR10KIsLoading_isLoading(false);   //!< Can disable the GUI, so no commands gets sent before setup is complete.

    inline extern std::atomic<bool> somethingUnexpectedHappend(false);  //!< If this is true, then the hooks will shut down and display a pop up.
    inline extern std::atomic<bool> comConnectionLost(false);           //!< Set by the COM reader thread when reading from the port keeps failing (e.g. USB adapter unplugged). Handled on the GUI thread.

    inline extern std::unique_ptr<std::thread> m_pThreadReconnecting = nullptr; //!< Runs LR10KControlSystem::StartReconnect() work off the GUI thread.
    inline extern std::atomic<bool> reconnectInProgress(false);         //!< True while a reconnect is running.

    /**
        @brief  Milliseconds from a monotonic clock (not affected by changes to the PC clock).
    **/
    inline long long SteadyNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    inline extern std::atomic<long long> lastByteReceivedMs(0);         //!< SteadyNowMs() when the COM reader last received a byte. Used by the watchdog.
    inline extern std::atomic<bool> lr10kNotResponding(false);          //!< Set by the watchdog when the LR 10K stops answering. Shows a pop up.
    inline extern std::atomic<bool> watchdogProbeReplyPending(false);   //!< The next status message is the answer to a watchdog probe: update the status quietly.


    /**
        @brief  Wait until a detached thread reports that it has left its thread function.
        @param  running - The thread's "running" flag.
        @param  timeout - Maximum time to wait.
        @retval false if the thread was still running after the timeout.
    **/
    inline bool WaitForThreadExit(const std::atomic<bool>& running, std::chrono::milliseconds timeout = std::chrono::milliseconds(1500))
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (running.load())
        {
            if (std::chrono::steady_clock::now() >= deadline)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // ---------------------- Runs on seperate threads => ----------------------
    // -------------------------------------------------------------------------

    UINT __cdecl    ConnectingToLR10K();
    UINT __cdecl    SendSetupToLR10K();

    // -------------------------------------------------------------------------
    // ---------------------- <= Runs on seperate threads ----------------------
    // -------------------------------------------------------------------------

    bool            ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(int trackID, std::chrono::steady_clock::time_point start, bool configDone = true);
    bool            ThreadCanRun_InterpetWrittenRingBufferData();
    void            InterpretMessage(const LR10KProtocol::Message& message, const LR10KReadings& readings);

    void            TerminateUniqeThread(std::unique_ptr<std::thread>& m_pThread, std::atomic<bool>* stopFlag = nullptr);

    float           InterpetWrittenRingBufferData_getLoad();
    float           InterpetWrittenRingBufferData_getExtension();

    std::string     InterpetWrittenRingBufferData_getNice_load();
    std::string     InterpetWrittenRingBufferData_getNice_extension();

    void    InterpetWrittenRingBufferData_WriteGlobals(LPARAM lParam, int action = 0);

    // The load multiplier and the decoding are in LR10KMachine (LR10KReadings) and LR10KProtocol.h.


    void    InterpetWrittenRingBufferData_handling_overflow(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_status(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_warnErrorStatus(LPARAM lParam);
    void    warnErrorStatus_Reset();
    void    InterpetWrittenRingBufferData_handling_unknownData(LPARAM lParam);

    // -------------------------------------------------------------------------
    // ----------------------- Custom converter algorithm ----------------------
    // -------------------------------------------------------------------------
    float convertAlgorithmForExtension(float value);
    int figureOutHowManyDecimalPlacesToShowAccuracy(float value);
    const char* figureOutHowManyDecimalPlacesToShowAccuracy_char(float value);
    float figureOutHowManyDecimalPlacesToShowAccuracy_float(float value);
}
