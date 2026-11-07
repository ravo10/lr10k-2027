// Unit tests for SerialTrafficLog (main/LloydLR10KTensile/SerialTrafficLog.h).
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I main/LloydLR10KTensile tests/serial_traffic_log_test.cpp -o serial_traffic_log_test
//   ./serial_traffic_log_test
//
// Run automatically by .github/workflows/tests.yml.

#include "SerialTrafficLog.h"
#include "LR10KMachine.h"
#include "fake_serial_port.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using Dir = SerialTrafficLog::Direction;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

static std::filesystem::path tempFile(const char* name)
{
    return std::filesystem::temp_directory_path() / "lr10k_log_test" / name;
}

static std::string readAll(const std::filesystem::path& p)
{
    std::ifstream in(p);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static void test_write_and_read_back()
{
    const auto file = tempFile("basic.log");
    SerialTrafficLog log;
    CHECK(!log.IsActive());
    CHECK(log.Start(file, "Started on COM4, 9600 baud 8N1"));
    CHECK(log.IsActive());

    // Connect handshake: ask, answer
    const std::uint8_t ask[] = { 0xB6 };
    log.LogSent(ask, 1);
    log.LogReceived(0xB6);
    std::this_thread::sleep_for(40ms); // pause -> new line
    log.LogReceived(0x00);
    log.LogNote("COM4 closed");
    const std::uint8_t go[] = { 0x81, 0xB6, 0xC1, 0xC0, 0x9F };
    log.LogSent(go, sizeof(go));
    log.LogReceived(0xB6);
    log.LogReceived(0x41); // pending until Stop()
    log.Stop();
    CHECK(!log.IsActive());

    const std::string text = readAll(file);
    CHECK(text.find("# LR 10K serial traffic log") != std::string::npos);
    CHECK(text.find("# Started on COM4, 9600 baud 8N1") != std::string::npos);
    CHECK(text.find("COM4 closed") != std::string::npos);
    CHECK(text.find("TX  81 B6 C1 C0 9F") != std::string::npos);

    bool ok = false;
    const auto entries = SerialTrafficLog::ReadFile(file, &ok);
    CHECK(ok);
    CHECK(entries.size() == 5); // the note is a comment line, not an entry
    if (entries.size() == 5)
    {
        CHECK(entries[0].direction == Dir::Sent && entries[0].bytes == std::vector<std::uint8_t>({ 0xB6 }));
        CHECK(entries[1].direction == Dir::Received && entries[1].bytes == std::vector<std::uint8_t>({ 0xB6 }));
        CHECK(entries[2].direction == Dir::Received && entries[2].bytes == std::vector<std::uint8_t>({ 0x00 }));
        CHECK(entries[2].seconds >= entries[1].seconds + 0.03);
        CHECK(entries[3].direction == Dir::Sent && entries[3].bytes.size() == 5);
        CHECK(entries[4].direction == Dir::Received && entries[4].bytes == std::vector<std::uint8_t>({ 0xB6, 0x41 })); // was pending at Stop()
    }
}

static void test_long_messages_are_split()
{
    const auto file = tempFile("long.log");
    SerialTrafficLog log;
    CHECK(log.Start(file));
    std::vector<std::uint8_t> setup(40);
    for (std::size_t i = 0; i < setup.size(); i++) setup[i] = (std::uint8_t)i;
    log.LogSent(setup.data(), setup.size());
    for (int i = 0; i < 20; i++) log.LogReceived((std::uint8_t)(0xA0 + i)); // one burst, 20 bytes
    log.Stop();

    bool ok = false;
    const auto entries = SerialTrafficLog::ReadFile(file, &ok);
    CHECK(ok);
    CHECK(entries.size() == 5); // TX 16+16+8, RX 16+4
    std::vector<std::uint8_t> tx, rx;
    for (const auto& e : entries)
        (e.direction == Dir::Sent ? tx : rx).insert((e.direction == Dir::Sent ? tx : rx).end(), e.bytes.begin(), e.bytes.end());
    CHECK(tx == setup);
    CHECK(rx.size() == 20 && rx.front() == 0xA0 && rx.back() == 0xB3);
}

static void test_not_logging_does_nothing()
{
    SerialTrafficLog log;
    const std::uint8_t b[] = { 1, 2 };
    log.LogSent(b, 2);
    log.LogReceived(3);
    log.LogNote("x");
    log.FlushIdle();
    log.Stop();
    CHECK(!log.IsActive());

    // A file that cannot be created
    CHECK(!log.Start(std::filesystem::path()));
    CHECK(!log.IsActive());
}

static void test_flush_idle()
{
    const auto file = tempFile("idle.log");
    SerialTrafficLog log;
    CHECK(log.Start(file));
    log.LogReceived(0xB6);
    log.LogReceived(0x00);
    log.FlushIdle(); // too soon: still pending
    CHECK(readAll(file).find("RX  B6 00") == std::string::npos);
    std::this_thread::sleep_for(40ms);
    log.FlushIdle();
    CHECK(readAll(file).find("RX  B6 00") != std::string::npos);
    log.Stop();
}

static void test_threads_and_replay()
{
    // Log while the machine reads from a fake port and another thread sends, then replay the log
    const auto file = tempFile("session.log");
    SerialTrafficLog log;
    CHECK(log.Start(file, "fake session"));

    FakeSerialPort port;
    for (int i = 0; i < 200; i++) port.AddBytes({ 0xB6, (std::uint8_t)i });

    LR10KMachine machine;
    SerialReader::Callbacks callbacks;
    callbacks.onByte = [&](std::uint8_t b) { log.LogReceived(b); };
    std::atomic<int> messages{ 0 };
    machine.StartReading(port, [] { return true; }, callbacks);
    machine.StartInterpreter([&](const LR10KProtocol::Message&, const LR10KReadings&) { messages++; }, [] { return true; });

    std::thread sender([&] {
        for (int i = 0; i < 50; i++)
        {
            const std::uint8_t cmd[] = { 0xB6 };
            std::uint32_t e;
            machine.Send(cmd, 1, e);
            log.LogSent(cmd, 1);
            std::this_thread::sleep_for(1ms);
        }
    });
    sender.join();
    const auto end = std::chrono::steady_clock::now() + 3s;
    while (messages < 200 && std::chrono::steady_clock::now() < end) std::this_thread::sleep_for(1ms);
    machine.StopInterpreter();
    machine.StopReading();
    log.Stop();
    CHECK(messages == 200);

    bool ok = false;
    const auto entries = SerialTrafficLog::ReadFile(file, &ok);
    CHECK(ok);
    std::vector<std::uint8_t> received;
    int sent = 0;
    for (const auto& e : entries)
    {
        if (e.direction == Dir::Received) received.insert(received.end(), e.bytes.begin(), e.bytes.end());
        else sent += (int)e.bytes.size();
    }
    CHECK(sent == 50);
    CHECK(received.size() == 400);

    // Replay the recorded RX bytes through a new fake port: the machine sees the same 200 messages
    FakeSerialPort replay;
    replay.AddBytes(received);
    LR10KMachine machine2;
    std::atomic<int> replayed{ 0 };
    machine2.StartReading(replay, [] { return true; });
    machine2.StartInterpreter([&](const LR10KProtocol::Message& m, const LR10KReadings&) { if (m.type == LR10KProtocol::Message::Type::Status) replayed++; }, [] { return true; });
    const auto end2 = std::chrono::steady_clock::now() + 3s;
    while (replayed < 200 && std::chrono::steady_clock::now() < end2) std::this_thread::sleep_for(1ms);
    machine2.StopInterpreter();
    machine2.StopReading();
    CHECK(replayed == 200);
}

int main()
{
    test_write_and_read_back();
    test_long_messages_are_split();
    test_not_logging_does_nothing();
    test_flush_idle();
    test_threads_and_replay();

    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "lr10k_log_test", ec);

    if (g_failures == 0)
        std::printf("serial_traffic_log_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
