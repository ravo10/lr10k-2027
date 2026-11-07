// A fake SerialPort for tests: plays back a script of bytes, quiet periods ("timeouts") and read
// errors like a real COM port with the LR 10K settings, and records everything written to it.
// It can also replay a recorded LR 10K session (add the recorded bytes with AddBytes).
#pragma once

#include "SerialPort.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class FakeSerialPort : public SerialPort
{
public:
    struct Event { ReadStatus status; std::uint8_t byte; std::uint32_t error; };

    void AddBytes(const std::vector<std::uint8_t>& bytes) { std::lock_guard<std::mutex> l(m); for (auto b : bytes) script.push_back({ ReadStatus::Byte, b, 0 }); }
    void AddTimeout() { std::lock_guard<std::mutex> l(m); script.push_back({ ReadStatus::Timeout, 0, 0 }); }
    void AddError(std::uint32_t code) { std::lock_guard<std::mutex> l(m); script.push_back({ ReadStatus::Error, 0, code }); }
    std::size_t Remaining() { std::lock_guard<std::mutex> l(m); return script.size(); }

    bool IsOpen() const override { return open; }
    ReadStatus ReadByte(std::uint8_t& out, std::uint32_t& errorCode) override
    {
        reads++;
        Event e{ ReadStatus::Timeout, 0, 0 };
        {
            std::lock_guard<std::mutex> l(m);
            if (!script.empty()) { e = script.front(); script.pop_front(); }
        }
        if (e.status == ReadStatus::Timeout)
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // a real port waits up to ~100 ms here
        errorCode = e.status == ReadStatus::Error ? e.error : 0;
        if (e.status == ReadStatus::Byte) out = e.byte;
        return e.status;
    }
    std::size_t Write(const std::uint8_t* data, std::size_t size, std::uint32_t& errorCode) override
    {
        errorCode = 0;
        std::lock_guard<std::mutex> l(m);
        written.insert(written.end(), data, data + size);
        return size;
    }
    std::vector<std::uint8_t> Written() { std::lock_guard<std::mutex> l(m); return written; }
    void DiscardBuffers() override {}
    bool Close() override { open = false; return true; }

    std::atomic<int> reads{ 0 };
    bool open = true;

private:
    std::mutex m;
    std::deque<Event> script;
    std::vector<std::uint8_t> written;
};
