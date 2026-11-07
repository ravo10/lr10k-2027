/**

    @file      SerialReader.h
    @brief     Background thread that copies bytes from a SerialPort into the ring buffer.
    @details   Portable (std::thread, no Windows headers) and unit tested with a fake port
               (tests/serial_reader_test.cpp). Replaces the detached reader thread: this one is
               owned by the object, so Stop() can wait until it has really finished before the
               port is closed or a new connection starts.

               Connection loss: a few failed reads in a row (e.g. USB adapter unplugged) end the
               thread and call onConnectionLost. A normal stop never reports a loss.

**/
#pragma once

#include "RingBuffer.h"
#include "SerialPort.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

class SerialReader
{
public:
    //! Optional notifications, called on the reader thread. They must not call Stop().
    struct Callbacks
    {
        std::function<void(std::uint8_t)> onByte;                                   //!< Every byte received (e.g. for the watchdog's "last byte" time).
        std::function<void(std::uint32_t lastError, int failedReads)> onConnectionLost; //!< Reading kept failing.
        std::function<void()> onExit;                                               //!< The thread is about to end (any reason).
    };

    static constexpr int maxConsecutiveReadErrors = 3;                        //!< Failed reads in a row that count as "connection lost".
    static constexpr std::chrono::milliseconds errorRetryDelay{ 50 };         //!< Pause after a failed read before trying again.
    static constexpr std::chrono::milliseconds bufferFullWaitSlice{ 20 };     //!< How often to re-check "keep running" while the buffer is full.

    SerialReader() = default;
    SerialReader(const SerialReader&) = delete;
    SerialReader& operator=(const SerialReader&) = delete;
    ~SerialReader() { Stop(); }

    /**
        @brief  Start reading @p port into @p buffer on a new thread.
        @param  keepRunning - Checked before every read; the thread ends when it returns false
                              (lets the program stop it with its existing flags).
        @details Any previous thread is stopped and waited for first.
    **/
    void Start(SerialPort& port, CRingBuffer& buffer, std::function<bool()> keepRunning, Callbacks callbacks = {})
    {
        Stop();
        m_stop.store(false);
        m_running.store(true);
        m_thread = std::thread(&SerialReader::Run, this, &port, &buffer, std::move(keepRunning), std::move(callbacks));
    }

    //! Ask the thread to end soon (within one port read timeout). Does not wait. Safe from any thread.
    void RequestStop() { m_stop.store(true); }

    /**
        @brief  Stop the thread and wait until it has ended (at most about one port read timeout).
        @details If called on the reader thread itself, it only requests the stop.
    **/
    void Stop()
    {
        m_stop.store(true);
        if (!m_thread.joinable())
            return;
        if (m_thread.get_id() == std::this_thread::get_id())
        {
            m_thread.detach(); // Cannot wait for ourselves; the loop ends on its own
            return;
        }
        m_thread.join();
    }

    //! True while the reader thread is running.
    bool IsRunning() const { return m_running.load(); }

private:
    void Run(SerialPort* port, CRingBuffer* buffer, std::function<bool()> keepRunning, Callbacks callbacks)
    {
        auto shouldRun = [&] { return !m_stop.load() && (!keepRunning || keepRunning()); };

        int consecutiveReadErrors = 0;
        std::uint8_t byte = 0;
        std::uint32_t error = 0;

        while (shouldRun())
        {
            const SerialPort::ReadStatus status = port->ReadByte(byte, error);

            if (status == SerialPort::ReadStatus::Byte)
            {
                consecutiveReadErrors = 0;
                if (callbacks.onByte)
                    callbacks.onByte(byte);

                // Wait for space while allowed to run (the consumer may be slower for a moment)
                while (shouldRun() && !buffer->WaitWrite(byte, bufferFullWaitSlice)) {}
                continue;
            }

            if (status == SerialPort::ReadStatus::Timeout)
            {
                consecutiveReadErrors = 0; // No data right now: normal
                continue;
            }

            // Read error
            if (++consecutiveReadErrors >= maxConsecutiveReadErrors)
            {
                // A normal disconnect also makes reads fail: only report a loss nobody asked for
                if (shouldRun() && callbacks.onConnectionLost)
                    callbacks.onConnectionLost(error, consecutiveReadErrors);
                break;
            }
            std::this_thread::sleep_for(errorRetryDelay);
        }

        if (callbacks.onExit)
            callbacks.onExit();
        m_running.store(false);
    }

    std::thread m_thread;
    std::atomic<bool> m_stop{ true };
    std::atomic<bool> m_running{ false };
};
