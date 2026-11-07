// A fake LR 10K that answers like a recorded session (a "Log serial traffic to file" log).
//
// The recording is a list of writes from the PC (TX) and the bytes the LR 10K sent (RX). When the
// program writes something, the device looks for the next recorded TX with exactly the same bytes
// and then sends the RX bytes that followed it in the recording. A write that does not match any
// later recorded TX is collected in Mismatches(): the program sent something the real session did not.
#pragma once

#include "SerialPort.h"
#include "SerialTrafficLog.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

class RecordedDevice : public SerialPort
{
public:
    struct Record { bool isTx; std::vector<std::uint8_t> bytes; };

    explicit RecordedDevice(const std::filesystem::path& logFile)
    {
        bool ok = false;
        const auto entries = SerialTrafficLog::ReadFile(logFile, &ok);
        m_loaded = ok && !entries.empty();

        // A long write is logged as several 16-byte lines with the same time: join them again
        double lastTxTime = -1.0;
        for (const auto& e : entries)
        {
            const bool isTx = e.direction == SerialTrafficLog::Direction::Sent;
            if (isTx && !m_records.empty() && m_records.back().isTx && e.seconds == lastTxTime
                && m_records.back().bytes.size() % SerialTrafficLog::maxBytesPerLine == 0)
                m_records.back().bytes.insert(m_records.back().bytes.end(), e.bytes.begin(), e.bytes.end());
            else
                m_records.push_back({ isTx, e.bytes });
            if (isTx) lastTxTime = e.seconds;
        }
    }

    bool Loaded() const { return m_loaded; }

    //! All recorded writes from the PC, in order.
    std::vector<std::vector<std::uint8_t>> RecordedWrites() const
    {
        std::vector<std::vector<std::uint8_t>> out;
        for (const auto& r : m_records) if (r.isTx) out.push_back(r.bytes);
        return out;
    }

    //! All recorded bytes from the LR 10K, in order.
    std::vector<std::uint8_t> RecordedReplies() const
    {
        std::vector<std::uint8_t> out;
        for (const auto& r : m_records) if (!r.isTx) out.insert(out.end(), r.bytes.begin(), r.bytes.end());
        return out;
    }

    std::vector<std::vector<std::uint8_t>> Writes() { std::lock_guard<std::mutex> l(m_mutex); return m_writes; }
    std::vector<std::vector<std::uint8_t>> Mismatches() { std::lock_guard<std::mutex> l(m_mutex); return m_mismatches; }

    // SerialPort
    bool IsOpen() const override { return true; }

    ReadStatus ReadByte(std::uint8_t& out, std::uint32_t& errorCode) override
    {
        errorCode = 0;
        {
            std::lock_guard<std::mutex> l(m_mutex);
            if (!m_pending.empty())
            {
                out = m_pending.front();
                m_pending.pop_front();
                return ReadStatus::Byte;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // a real port waits up to ~100 ms
        return ReadStatus::Timeout;
    }

    std::size_t Write(const std::uint8_t* data, std::size_t size, std::uint32_t& errorCode) override
    {
        errorCode = 0;
        std::lock_guard<std::mutex> l(m_mutex);
        const std::vector<std::uint8_t> written(data, data + size);
        m_writes.push_back(written);

        // Find the next recorded write with the same bytes
        std::size_t k = m_position;
        while (k < m_records.size() && !(m_records[k].isTx && m_records[k].bytes == written))
            k++;
        if (k == m_records.size())
        {
            m_mismatches.push_back(written);
            return size;
        }

        // Answer with everything the LR 10K sent until the next recorded write (also RX before it, if
        // recorded writes were skipped)
        for (std::size_t i = m_position; i < m_records.size(); i++)
        {
            if (i > k && m_records[i].isTx) { m_position = i; return size; }
            if (!m_records[i].isTx)
                m_pending.insert(m_pending.end(), m_records[i].bytes.begin(), m_records[i].bytes.end());
        }
        m_position = m_records.size();
        return size;
    }

    void DiscardBuffers() override { std::lock_guard<std::mutex> l(m_mutex); m_pending.clear(); }
    bool Close() override { return true; }

private:
    bool m_loaded = false;
    std::vector<Record> m_records;
    std::size_t m_position = 0;

    std::mutex m_mutex;
    std::deque<std::uint8_t> m_pending;
    std::vector<std::vector<std::uint8_t>> m_writes;
    std::vector<std::vector<std::uint8_t>> m_mismatches;
};
