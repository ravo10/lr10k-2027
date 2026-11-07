// Unit tests for SerialReader (main/LloydLR10KTensile/SerialReader.h) with a fake serial port.
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I main/LloydLR10KTensile tests/serial_reader_test.cpp -o serial_reader_test
//   ./serial_reader_test
//
// Run automatically by .github/workflows/tests.yml.
//
// FakeSerialPort plays back a script of bytes, quiet periods ("timeouts") and read errors, like a
// real COM port with the LR 10K settings. The same fake can later replay recorded LR 10K sessions.

#include "SerialReader.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

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
            std::this_thread::sleep_for(2ms); // a real port waits up to ~100 ms here
        errorCode = e.status == ReadStatus::Error ? e.error : 0;
        if (e.status == ReadStatus::Byte) out = e.byte;
        return e.status;
    }
    std::size_t Write(const std::uint8_t*, std::size_t size, std::uint32_t& errorCode) override { errorCode = 0; return size; }
    void DiscardBuffers() override {}
    bool Close() override { open = false; return true; }

    std::atomic<int> reads{ 0 };
    bool open = true;

private:
    std::mutex m;
    std::deque<Event> script;
};

static bool waitFor(const std::function<bool()>& cond, std::chrono::milliseconds timeout = 2000ms)
{
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (!cond())
    {
        if (std::chrono::steady_clock::now() > end) return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

static std::vector<std::uint8_t> drain(CRingBuffer& buffer)
{
    std::vector<std::uint8_t> out;
    std::uint8_t b;
    while (buffer.TryRead(b, 0ms)) out.push_back(b);
    return out;
}

static void test_bytes_arrive_in_order()
{
    FakeSerialPort port;
    CRingBuffer buffer;
    port.AddBytes({ 0xB6, 0x00, 0xE1, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00 });
    port.AddTimeout();
    port.AddBytes({ 0xBF, 0x01 });

    std::atomic<int> onByteCalls{ 0 };
    std::atomic<bool> exited{ false };
    SerialReader reader;
    SerialReader::Callbacks cb;
    cb.onByte = [&](std::uint8_t) { onByteCalls++; };
    cb.onExit = [&] { exited = true; };
    reader.Start(port, buffer, [] { return true; }, cb);

    CHECK(waitFor([&] { return buffer.Size() == 11; }));
    CHECK(reader.IsRunning());
    reader.Stop();
    CHECK(!reader.IsRunning());
    CHECK(exited);
    CHECK(onByteCalls == 11);
    const std::vector<std::uint8_t> expected = { 0xB6, 0x00, 0xE1, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00, 0xBF, 0x01 };
    CHECK(drain(buffer) == expected);
}

static void test_three_errors_mean_connection_lost()
{
    FakeSerialPort port;
    CRingBuffer buffer;
    port.AddBytes({ 0x01 });
    port.AddError(22); // e.g. ERROR_BAD_COMMAND after the USB adapter was pulled
    port.AddError(22);
    port.AddError(22);
    port.AddBytes({ 0x02 }); // never read: the thread has ended

    std::atomic<int> lostCalls{ 0 };
    std::atomic<std::uint32_t> lastError{ 0 };
    std::atomic<int> failedReads{ 0 };
    std::atomic<bool> exited{ false };
    SerialReader reader;
    SerialReader::Callbacks cb;
    cb.onConnectionLost = [&](std::uint32_t e, int n) { lostCalls++; lastError = e; failedReads = n; };
    cb.onExit = [&] { exited = true; };
    reader.Start(port, buffer, [] { return true; }, cb);

    CHECK(waitFor([&] { return !reader.IsRunning(); }));
    CHECK(lostCalls == 1);
    CHECK(lastError == 22u);
    CHECK(failedReads == SerialReader::maxConsecutiveReadErrors);
    CHECK(exited);
    CHECK(port.Remaining() == 1);
    CHECK(drain(buffer) == std::vector<std::uint8_t>({ 0x01 }));
    reader.Stop(); // joining a finished thread is fine
}

static void test_occasional_errors_are_tolerated()
{
    // Two errors, then data again: a glitch, not a lost connection
    FakeSerialPort port;
    CRingBuffer buffer;
    port.AddError(5);
    port.AddError(5);
    port.AddBytes({ 0xAA });
    port.AddError(5);
    port.AddError(5);
    port.AddTimeout(); // a timeout also resets the count
    port.AddError(5);
    port.AddBytes({ 0xBB });

    std::atomic<int> lostCalls{ 0 };
    SerialReader reader;
    SerialReader::Callbacks cb;
    cb.onConnectionLost = [&](std::uint32_t, int) { lostCalls++; };
    reader.Start(port, buffer, [] { return true; }, cb);

    CHECK(waitFor([&] { return buffer.Size() == 2; }));
    reader.Stop();
    CHECK(lostCalls == 0);
    CHECK(drain(buffer) == std::vector<std::uint8_t>({ 0xAA, 0xBB }));
}

static void test_normal_stop_is_not_a_loss()
{
    // The program's own stop flag ends the thread; failing reads after that are not reported
    FakeSerialPort port;
    CRingBuffer buffer;
    std::atomic<bool> comActive{ true };
    std::atomic<int> lostCalls{ 0 };
    SerialReader reader;
    SerialReader::Callbacks cb;
    cb.onConnectionLost = [&](std::uint32_t, int) { lostCalls++; };
    reader.Start(port, buffer, [&] { return comActive.load(); }, cb);

    CHECK(waitFor([&] { return port.reads > 5; })); // reading (only timeouts)
    comActive = false;                              // like m_pThreadReadCom_stopFlag = true
    port.AddError(6); port.AddError(6); port.AddError(6);
    CHECK(waitFor([&] { return !reader.IsRunning(); }));
    reader.Stop();
    CHECK(lostCalls == 0);
}

static void test_stop_while_buffer_is_full()
{
    // Nobody reads the buffer: the reader must still stop promptly instead of hanging on a full buffer
    FakeSerialPort port;
    CRingBuffer buffer;
    std::vector<std::uint8_t> lots(CRingBuffer::bufferSize + 50, 0x55);
    port.AddBytes(lots);

    SerialReader reader;
    reader.Start(port, buffer, [] { return true; });
    CHECK(waitFor([&] { return buffer.Size() == CRingBuffer::bufferSize; }));

    const auto t0 = std::chrono::steady_clock::now();
    reader.Stop();
    CHECK(std::chrono::steady_clock::now() - t0 < 500ms);
    CHECK(!reader.IsRunning());
}

static void test_restart_and_destructor()
{
    FakeSerialPort port;
    CRingBuffer buffer;
    {
        SerialReader reader;
        port.AddBytes({ 1, 2 });
        reader.Start(port, buffer, [] { return true; });
        CHECK(waitFor([&] { return buffer.Size() == 2; }));

        // Start again without Stop(): the old thread is stopped and waited for first
        port.AddBytes({ 3 });
        reader.Start(port, buffer, [] { return true; });
        CHECK(waitFor([&] { return buffer.Size() == 3; }));
    } // destructor stops the thread
    CHECK(drain(buffer) == std::vector<std::uint8_t>({ 1, 2, 3 }));
}

int main()
{
    test_bytes_arrive_in_order();
    test_three_errors_mean_connection_lost();
    test_occasional_errors_are_tolerated();
    test_normal_stop_is_not_a_loss();
    test_stop_while_buffer_is_full();
    test_restart_and_destructor();

    if (g_failures == 0)
        std::printf("serial_reader_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
