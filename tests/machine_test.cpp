// Unit tests for LR10KMachine (main/LloydLR10KTensile/LR10KMachine.h) with a fake serial port.
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I main/LloydLR10KTensile tests/machine_test.cpp -o machine_test
//   ./machine_test
//
// Run automatically by .github/workflows/tests.yml.

#include "LR10KMachine.h"
#include "fake_serial_port.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using LR10KProtocol::Message;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

static bool waitFor(const std::function<bool()>& cond, std::chrono::milliseconds timeout = 3000ms)
{
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (!cond())
    {
        if (std::chrono::steady_clock::now() > end) return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

// Collects messages from the interpreter thread
struct Collector
{
    std::mutex m;
    std::vector<Message> messages;
    void operator()(const Message& msg) { std::lock_guard<std::mutex> l(m); messages.push_back(msg); }
    std::size_t Count() { std::lock_guard<std::mutex> l(m); return messages.size(); }
    std::vector<Message> Get() { std::lock_guard<std::mutex> l(m); return messages; }
};

static void test_port_to_messages()
{
    // A short live-data stream as the LR 10K sends it: status, two data packages, overflow, warning
    FakeSerialPort port;
    port.AddBytes({ 0xB6, 0x00 });
    port.AddBytes({ 0xF0, 0x10, 0x00, 0x01, 0x00, 0x00 });
    port.AddTimeout();
    port.AddBytes({ 0xF0, 0x20, 0x00, 0x02, 0x00, 0x00 });
    port.AddBytes({ 0xB1, 0x03, 0xBF, 0x01 });

    LR10KMachine machine;
    Collector collected;
    std::atomic<bool> interpreterExited{ false };

    machine.StartReading(port, [] { return true; });
    machine.StartInterpreter([&](const Message& m, const LR10KReadings&) { collected(m); }, [] { return true; }, [&] { interpreterExited = true; });

    CHECK(waitFor([&] { return collected.Count() == 5; }));
    const auto msgs = collected.Get();
    CHECK(msgs.size() == 5);
    if (msgs.size() == 5)
    {
        CHECK(msgs[0].type == Message::Type::Status && msgs[0].Value() == 0x00);
        CHECK(msgs[1].type == Message::Type::DataPackage);
        CHECK(LR10KProtocol::DecodeLoad(msgs[1].data[0], msgs[1].data[1], 1) == 16);   // 0x810 - 0x800
        CHECK(LR10KProtocol::DecodeDistanceDelta(msgs[1].data[2], msgs[1].data[3]) == 4);
        CHECK(msgs[2].type == Message::Type::DataPackage);
        CHECK(LR10KProtocol::DecodeLoad(msgs[2].data[0], msgs[2].data[1], 1) == 32);
        CHECK(LR10KProtocol::DecodeDistanceDelta(msgs[2].data[2], msgs[2].data[3]) == 8);
        CHECK(msgs[3].type == Message::Type::Overflow && msgs[3].Value() == 0x03);
        CHECK(msgs[4].type == Message::Type::WarnError && msgs[4].Value() == 0x01);
    }

    CHECK(machine.IsReading() && machine.IsInterpreting());
    machine.StopInterpreter();
    CHECK(!machine.IsInterpreting());
    CHECK(interpreterExited);
    machine.StopReading();
    CHECK(!machine.IsReading());
}

static void test_send_goes_to_port()
{
    FakeSerialPort port;
    LR10KMachine machine;

    std::uint32_t error = 0;
    const std::uint8_t setupStart[] = { 0xB0 };
    CHECK(machine.Send(setupStart, 1, error) == 0); // no port yet
    CHECK(error != 0);

    machine.StartReading(port, [] { return true; });
    const std::uint8_t go[] = { 0x81, 0xB6, 0xC1, 0xC0, 0x9F }; // Write_Go
    CHECK(machine.Send(go, sizeof(go), error) == sizeof(go));
    CHECK(error == 0);
    CHECK(port.Written() == std::vector<std::uint8_t>(go, go + sizeof(go)));
    machine.StopReading();
}

static void test_restart_interpreter_never_splits_bytes()
{
    // Restarting the interpreter (reconnect) must stop the old one first: every message arrives once
    FakeSerialPort port;
    LR10KMachine machine;
    Collector first, second;

    machine.StartReading(port, [] { return true; });
    machine.StartInterpreter([&](const Message& m, const LR10KReadings&) { first(m); }, [] { return true; });
    port.AddBytes({ 0xB6, 0x41 });
    CHECK(waitFor([&] { return first.Count() == 1; }));

    machine.StartInterpreter([&](const Message& m, const LR10KReadings&) { second(m); }, [] { return true; });
    for (int i = 0; i < 50; i++)
        port.AddBytes({ 0xB6, (std::uint8_t)i });
    CHECK(waitFor([&] { return second.Count() == 50; }));
    std::this_thread::sleep_for(50ms);
    CHECK(first.Count() == 1);   // the old interpreter got nothing after the restart
    CHECK(second.Count() == 50); // and nothing was lost

    machine.StopInterpreter();
    machine.StopReading();
}

static void test_program_flags_stop_threads()
{
    // The program's own stop flags (m_pThreadReadCom_stopFlag / m_pThreadReadLR10K_stopFlag) end the threads
    FakeSerialPort port;
    LR10KMachine machine;
    std::atomic<bool> comActive{ true }, interpreterActive{ true };

    machine.StartReading(port, [&] { return comActive.load(); });
    machine.StartInterpreter([](const Message&, const LR10KReadings&) {}, [&] { return interpreterActive.load() && comActive.load(); });
    CHECK(waitFor([&] { return port.reads > 3; }));

    interpreterActive = false;
    CHECK(waitFor([&] { return !machine.IsInterpreting(); }, 500ms));
    CHECK(machine.IsReading());

    comActive = false;
    CHECK(waitFor([&] { return !machine.IsReading(); }, 500ms));
    machine.StopInterpreter();
    machine.StopReading();
}

static void test_connection_loss_stops_interpreter()
{
    // As in the app: the reader's onExit asks the interpreter to stop
    FakeSerialPort port;
    LR10KMachine machine;
    std::atomic<int> lost{ 0 };

    SerialReader::Callbacks callbacks;
    callbacks.onConnectionLost = [&](std::uint32_t, int) { lost++; };
    callbacks.onExit = [&] { machine.RequestStopInterpreter(); };

    machine.StartInterpreter([](const Message&, const LR10KReadings&) {}, [] { return true; });
    machine.StartReading(port, [] { return true; }, callbacks);
    port.AddBytes({ 0xB6, 0x00 });
    port.AddError(22); port.AddError(22); port.AddError(22);

    CHECK(waitFor([&] { return !machine.IsReading(); }));
    CHECK(waitFor([&] { return !machine.IsInterpreting(); }, 500ms));
    CHECK(lost == 1);
    machine.StopInterpreter();
    machine.StopReading();
}

static void test_readings()
{
    // The machine decodes every message into its readings (the same decoding as LR10KProtocol)
    FakeSerialPort port;
    port.AddBytes({ 0xF0, 0x10, 0x00, 0x01, 0x00, 0x02 });  // load +16, distance +4, extensometer 2
    port.AddBytes({ 0xF0, 0x20, 0x7F, 0x7F, 0x00, 0x00 });  // load +32, distance -4
    port.AddBytes({ 0xF0, 0x05, 0x00, 0x02, 0x00, 0x00 });  // load +5, distance +8
    port.AddBytes({ 0xB6, 0x41 });                          // status: going up
    port.AddBytes({ 0xBF, 0x02 });                          // warning: tension overload stop
    port.AddBytes({ 0xB1, 0x02, 0x42 });                    // multiplier 2, one unknown byte

    LR10KMachine machine;
    std::mutex m;
    std::vector<LR10KReadings> seen;
    machine.StartReading(port, [] { return true; });
    machine.StartInterpreter([&](const Message&, const LR10KReadings& r) { std::lock_guard<std::mutex> l(m); seen.push_back(r); }, [] { return true; });
    CHECK(waitFor([&] { return machine.Readings().unknownBytes == 1; }));
    machine.StopInterpreter();
    machine.StopReading();

    const LR10KReadings r = machine.Readings();
    CHECK(r.dataPackages == 3);
    CHECK(r.load == 5);
    CHECK(r.distanceDelta == 8);
    CHECK(r.distance == 4 - 4 + 8);
    CHECK(r.extensometer == 0);
    CHECK(r.hasStatus && r.status == 0x41 && r.statusMessages == 1);
    CHECK(r.hasWarnError && r.warnError == 0x02 && r.warnErrorMessages == 1);
    CHECK(r.loadMultiplier == 2 && r.overflowMessages == 1);
    CHECK(r.unknownBytes == 1);

    // The handler got the readings right after each message
    CHECK(seen.size() == 7);
    if (seen.size() == 7)
    {
        CHECK(seen[0].load == 16 && seen[0].distance == 4 && seen[0].extensometer == 2);
        CHECK(seen[1].load == 32 && seen[1].distance == 0);
    }

    // Zero point: the distance starts from 0 again
    machine.ResetDistance();
    CHECK(machine.Readings().distance == 0);
    CHECK(machine.Readings().load == 5); // the rest is kept

    // The load multiplier is used for the next data package (multiplier 2: 0xC00 -> +2048)
    LR10KProtocol::Message pkg;
    pkg.type = Message::Type::DataPackage;
    pkg.data = { 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 };
    CHECK(machine.Apply(pkg).load == 2048);
}

static void test_destructor_stops_everything()
{
    FakeSerialPort port;
    {
        LR10KMachine machine;
        machine.StartReading(port, [] { return true; });
        machine.StartInterpreter([](const Message&, const LR10KReadings&) {}, [] { return true; });
        port.AddBytes({ 0xB6, 0x00, 0xB6, 0x41 });
    } // must not hang or crash
    CHECK(true);
}

int main()
{
    test_port_to_messages();
    test_send_goes_to_port();
    test_restart_interpreter_never_splits_bytes();
    test_program_flags_stop_threads();
    test_connection_loss_stops_interpreter();
    test_readings();
    test_destructor_stops_everything();

    if (g_failures == 0)
        std::printf("machine_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
