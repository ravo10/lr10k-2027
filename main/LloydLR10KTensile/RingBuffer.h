/**

    @file      RingBuffer.h
    @brief     Thread-safe ring buffer between the COM reader thread and its consumers.
    @details   Portable (no Windows headers) so it can be unit tested on any platform,
               see tests/ring_buffer_test.cpp. The blocking Read()/Write() wrappers that
               honour the LR 10K thread stop flags are implemented in RingBuffer.cpp.
    @author    Rasmus Ullestad Relling (orginal logic by Olaf Hallan Graven)
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/
#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>

/**

    @class   CRingBuffer
    @brief   Single-producer/consumer FIFO of bytes received from the LR 10K.
    @details All state is guarded by one mutex. Waiting uses condition variables,
             so readers and writers sleep instead of spinning, and a read never
             returns a byte that was not written.
    @note    Original logic by: Olaf Hallan Graven

**/
class CRingBuffer
{
public:
    static constexpr std::size_t bufferSize{ 200 };  //!< Maximum number of unread bytes held.

    CRingBuffer() = default;
    CRingBuffer(const CRingBuffer&) = delete;
    CRingBuffer& operator=(const CRingBuffer&) = delete;

    /**
        @brief  Append a byte without waiting.
        @retval false if the buffer is full (the byte is dropped).
    **/
    bool TryWrite(std::uint8_t element)
    {
        {
            std::lock_guard<std::mutex> lock(m_Protect);
            if (m_uCount == bufferSize)
                return false;
            pushLocked(element);
        }
        m_DataAvailable.notify_all();
        return true;
    }

    /**
        @brief  Append a byte, waiting up to @p timeout for free space.
        @retval false if the buffer stayed full for the whole timeout.
    **/
    bool WaitWrite(std::uint8_t element, std::chrono::milliseconds timeout)
    {
        {
            std::unique_lock<std::mutex> lock(m_Protect);
            if (!m_SpaceAvailable.wait_for(lock, timeout, [this] { return m_uCount < bufferSize; }))
                return false;
            pushLocked(element);
        }
        m_DataAvailable.notify_all();
        return true;
    }

    /**
        @brief  Take the oldest byte, waiting up to @p timeout for one to arrive.
        @param  out     - Receives the byte. Left unchanged on timeout.
        @param  timeout - Maximum time to wait. Zero means "only if data is already there".
        @retval false on timeout (no data).
    **/
    bool TryRead(std::uint8_t& out, std::chrono::milliseconds timeout)
    {
        {
            std::unique_lock<std::mutex> lock(m_Protect);
            if (!m_DataAvailable.wait_for(lock, timeout, [this] { return m_uCount > 0; }))
                return false;
            out = m_uData[m_uRead];
            m_uRead = (m_uRead + 1) % bufferSize;
            --m_uCount;
        }
        m_SpaceAvailable.notify_all();
        return true;
    }

    /**
        @brief  Check if there is any unread data in the buffer.
    **/
    bool DataAvailable() const
    {
        std::lock_guard<std::mutex> lock(m_Protect);
        return m_uCount > 0;
    }

    /**
        @brief  Number of unread bytes.
    **/
    std::size_t Size() const
    {
        std::lock_guard<std::mutex> lock(m_Protect);
        return m_uCount;
    }

    /**
        @brief  Drop all unread data and reset to the initial state.
    **/
    void ClearAllData()
    {
        {
            std::lock_guard<std::mutex> lock(m_Protect);
            m_uData.fill(0);
            m_uRead = 0;
            m_uWrite = 0;
            m_uCount = 0;
        }
        m_SpaceAvailable.notify_all();
    }

    /**
        @brief  Blocking write used by the COM reader thread. Waits for space while the
                COM thread is allowed to run (see RingBuffer.cpp).
        @param  Element     - Byte to store.
        @param  NonBlocking - If true, behaves like TryWrite().
        @retval false if the byte could not be stored (buffer full and the thread was stopped).
    **/
    bool Write(std::uint8_t Element, bool NonBlocking = false);

    /**
        @brief  Blocking read. Waits for data while the COM thread is allowed to run
                (see RingBuffer.cpp).
        @param  configDone - Passed to the thread stop check (enables the "stuck while connecting" guard).
        @retval The byte read, or 0x00 if the thread was stopped before any data arrived.
    **/
    std::uint8_t Read(bool configDone = true);

private:
    void pushLocked(std::uint8_t element)
    {
        m_uData[m_uWrite] = element;
        m_uWrite = (m_uWrite + 1) % bufferSize;
        ++m_uCount;
    }

    std::array<std::uint8_t, bufferSize> m_uData{};  //!< Container for the buffer data.
    std::size_t m_uRead{ 0 };                        //!< Index of the next byte to read.
    std::size_t m_uWrite{ 0 };                       //!< Index where the next byte is written.
    std::size_t m_uCount{ 0 };                       //!< Number of unread bytes.

    mutable std::mutex m_Protect;                    //!< Guards all members above.
    std::condition_variable m_DataAvailable;         //!< Signalled when a byte is written.
    std::condition_variable m_SpaceAvailable;        //!< Signalled when a byte is read or the buffer is cleared.
};
