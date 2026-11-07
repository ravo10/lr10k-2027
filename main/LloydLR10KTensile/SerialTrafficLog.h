/**

    @file      SerialTrafficLog.h
    @brief     Writes every byte sent to and received from the LR 10K to a text file, with timestamps.
    @details   Portable (no Windows or GUI code) and unit tested (tests/serial_traffic_log_test.cpp).
               Thread safe: bytes are logged from the reader thread and from whichever thread sends.

               File format (one line per burst of bytes; lines starting with '#' are comments):

                   # LR 10K serial traffic log
                   # time_s  dir  bytes (hex)   TX = PC -> LR 10K, RX = LR 10K -> PC
                      0.000  TX  B6
                      0.031  RX  B6
                      0.170  TX  C1 00

               Received bytes arrive one at a time; they are collected into one RX line until there is a
               pause, something is sent, or the line is full. ReadFile() reads a log back, e.g. to replay a
               recorded session through a fake serial port in a test.

**/
#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

class SerialTrafficLog
{
public:
    enum class Direction { Sent, Received };

    //! One line of a log read back with ReadFile().
    struct Entry
    {
        double seconds = 0.0;
        Direction direction = Direction::Sent;
        std::vector<std::uint8_t> bytes;
    };

    static constexpr std::size_t maxBytesPerLine = 16;                  //!< A received burst is split into lines of at most this many bytes.
    static constexpr std::chrono::milliseconds receiveGap{ 30 };        //!< A pause this long between received bytes starts a new line.

    SerialTrafficLog() = default;
    SerialTrafficLog(const SerialTrafficLog&) = delete;
    SerialTrafficLog& operator=(const SerialTrafficLog&) = delete;
    ~SerialTrafficLog() { Stop(); }

    /**
        @brief  Start logging to @p file (created or overwritten). Stops a running log first.
        @param  description - Extra header text, e.g. "Started 2026-11-07 13:05 on COM4, 9600 baud 8N1".
        @retval false if the file could not be created.
    **/
    bool Start(const std::filesystem::path& file, const std::string& description = "")
    {
        Stop();
        std::lock_guard<std::mutex> lock(m_mutex);

        std::error_code ec;
        if (file.has_parent_path())
            std::filesystem::create_directories(file.parent_path(), ec);

        m_file.open(file, std::ios::out | std::ios::trunc);
        if (!m_file.is_open())
            return false;

        m_path = file;
        m_start = Clock::now();
        m_pending.clear();

        m_file << "# LR 10K serial traffic log\n";
        if (!description.empty())
            m_file << "# " << description << "\n";
        m_file << "# time_s  dir  bytes (hex)   TX = PC -> LR 10K, RX = LR 10K -> PC\n";
        m_file.flush();
        return true;
    }

    //! Write any pending received bytes and close the file. Safe to call when not logging.
    void Stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open())
            return;
        FlushPendingLocked();
        m_file << "# Log stopped at " << FormatSeconds(SecondsSince(Clock::now())) << " s\n";
        m_file.close();
    }

    bool IsActive() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_file.is_open();
    }

    //! The file being written (or the last one).
    std::filesystem::path Path() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_path;
    }

    //! Log bytes sent to the LR 10K (one TX line). Does nothing when not logging.
    void LogSent(const std::uint8_t* data, std::size_t size)
    {
        if (size == 0) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) return;

        FlushPendingLocked(); // Keep the order: what came in before this send is written first
        WriteLineLocked(SecondsSince(Clock::now()), "TX", data, size);
    }

    //! Log one byte received from the LR 10K. Does nothing when not logging.
    void LogReceived(std::uint8_t byte)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) return;

        const auto now = Clock::now();
        if (!m_pending.empty() && (now - m_lastReceived >= receiveGap || m_pending.size() >= maxBytesPerLine))
            FlushPendingLocked();

        if (m_pending.empty())
            m_pendingStart = now;
        m_pending.push_back(byte);
        m_lastReceived = now;
    }

    //! Add a comment line (e.g. "COM4 opened"). Does nothing when not logging.
    void LogNote(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) return;
        FlushPendingLocked();
        m_file << "# " << FormatSeconds(SecondsSince(Clock::now())) << "  " << text << "\n";
        m_file.flush();
    }

    //! Write received bytes that have been waiting longer than the gap. Call regularly (e.g. once per frame).
    void FlushIdle()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open() && !m_pending.empty() && Clock::now() - m_lastReceived >= receiveGap)
            FlushPendingLocked();
    }

    /**
        @brief  Read a log file back (comment lines are skipped).
        @param  ok - Set to false if the file could not be opened or a line could not be parsed.
    **/
    static std::vector<Entry> ReadFile(const std::filesystem::path& file, bool* ok = nullptr)
    {
        std::vector<Entry> entries;
        std::ifstream in(file);
        bool good = in.is_open();

        std::string line;
        while (good && std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::size_t first = line.find_first_not_of(" \t");
            if (first == std::string::npos || line[first] == '#')
                continue;

            std::istringstream fields(line);
            Entry entry;
            std::string dir;
            if (!(fields >> entry.seconds >> dir) || (dir != "TX" && dir != "RX"))
            {
                good = false;
                break;
            }
            entry.direction = dir == "TX" ? Direction::Sent : Direction::Received;

            std::string hex;
            while (fields >> hex)
            {
                unsigned value = 0;
                if (hex.size() != 2 || std::sscanf(hex.c_str(), "%2x", &value) != 1)
                {
                    good = false;
                    break;
                }
                entry.bytes.push_back((std::uint8_t)value);
            }
            entries.push_back(entry);
        }

        if (ok) *ok = good;
        return entries;
    }

private:
    using Clock = std::chrono::steady_clock;

    double SecondsSince(Clock::time_point t) const { return std::chrono::duration<double>(t - m_start).count(); }

    static std::string FormatSeconds(double seconds)
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%8.3f", seconds);
        return text;
    }

    void FlushPendingLocked()
    {
        if (m_pending.empty()) return;
        WriteLineLocked(SecondsSince(m_pendingStart), "RX", m_pending.data(), m_pending.size());
        m_pending.clear();
    }

    void WriteLineLocked(double seconds, const char* direction, const std::uint8_t* data, std::size_t size)
    {
        // Long messages (e.g. a setup sequence) are split into several lines with the same time
        for (std::size_t offset = 0; offset < size; offset += maxBytesPerLine)
        {
            m_file << FormatSeconds(seconds) << "  " << direction << " ";
            const std::size_t end = (offset + maxBytesPerLine < size) ? offset + maxBytesPerLine : size;
            for (std::size_t i = offset; i < end; i++)
            {
                char hex[4];
                std::snprintf(hex, sizeof(hex), " %02X", data[i]);
                m_file << hex;
            }
            m_file << "\n";
        }
        m_file.flush(); // Keep the file complete even if the program crashes
    }

    mutable std::mutex m_mutex;
    std::ofstream m_file;
    std::filesystem::path m_path;
    Clock::time_point m_start{};
    Clock::time_point m_pendingStart{};
    Clock::time_point m_lastReceived{};
    std::vector<std::uint8_t> m_pending;
};
