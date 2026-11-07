// Unit tests for SerialReader (main/LloydLR10KTensile/SerialReader.h) with a fake serial port.
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I main/LloydLR10KTensile tests/serial_reader_test.cpp -o serial_reader_test
//   ./serial_reader_test
//
// Run automatically by .github/workflows/tests.yml.
//
// FakeSerialPort (fake_serial_port.h) plays back a script of bytes, quiet periods and read errors.

#include "SerialReader.h"
#include "fake_serial_port.h"

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
