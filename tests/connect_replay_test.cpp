// Replays a real "Connect PC" session (tests/data/connect_session_2026-11-07.log, recorded on the lab PC
// with "Log serial traffic to file") against the program's connect logic.
//
//   g++ -std=c++17 -O1 -g -pthread -I main/LloydLR10KTensile tests/connect_replay_test.cpp -o connect_replay_test
//   ./connect_replay_test [path to the log]
//
// Run automatically by .github/workflows/tests.yml (from the repository root).
//
// What it checks:
//  - The handshake and setup #1 (LR10KSequences::FirstSetup) complete against the recorded LR 10K and
//    the program sends exactly the bytes of the real session, in the same order.
//  - Without skipping the first step of setup #1, it gets stuck waiting for 0xB6 (the "stuck at 6 %" bug).
//  - The whole recorded byte stream is framed into messages without losing sync, and decodes sensibly.
//  - The recorded setup #2 message contains the expected settings.

#include "LR10KMachine.h"
#include "LR10KSequences.h"
#include "recorded_device.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <thread>
#include <utility>
#include <string>
#include <vector>

using namespace std::chrono_literals;
using LR10KProtocol::Message;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

static std::filesystem::path g_log = "tests/data/connect_session_2026-11-07.log";
static long long g_expectedDistance = 0; // computed by test_recorded_stream_frames_cleanly()

// Plays a fixed byte stream (no answers to writes)
class FakeStream : public SerialPort
{
public:
    explicit FakeStream(std::vector<std::uint8_t> bytes) : m_bytes(std::move(bytes)) {}
    bool IsOpen() const override { return true; }
    ReadStatus ReadByte(std::uint8_t& out, std::uint32_t& e) override
    {
        e = 0;
        if (m_pos < m_bytes.size()) { out = m_bytes[m_pos++]; return ReadStatus::Byte; }
        std::this_thread::sleep_for(1ms);
        return ReadStatus::Timeout;
    }
    std::size_t Write(const std::uint8_t*, std::size_t size, std::uint32_t& e) override { e = 0; return size; }
    void DiscardBuffers() override {}
    bool Close() override { return true; }
private:
    std::vector<std::uint8_t> m_bytes;
    std::size_t m_pos = 0;
};

static std::string hex(const std::vector<std::uint8_t>& b)
{
    std::string s;
    char t[4];
    for (auto x : b) { std::snprintf(t, sizeof(t), "%02X ", x); s += t; }
    return s;
}

// The program's setup #1: handshake, then the sequence without its first step (see w_Setup1)
static std::vector<std::pair<std::uint8_t, bool>> setupOneAfterHandshake()
{
    std::vector<std::pair<std::uint8_t, bool>> seq = LR10KSequences::FirstSetup();
    if (!seq.empty() && seq.front().first == 0xB6 && seq.front().second)
        seq.erase(seq.begin());
    return seq;
}

static void test_connect_matches_real_session()
{
    RecordedDevice device(g_log);
    CHECK(device.Loaded());
    if (!device.Loaded()) return;

    LR10KMachine machine;
    machine.StartReading(device, [] { return true; });
    auto send = [&](const std::uint8_t* data, std::size_t size) { std::uint32_t e; return device.Write(data, size, e) == size; };

    // 1. "Ready to connect?" -> the recorded LR 10K answers "B0 0B B6 00" to the first request
    const auto hs = machine.AskReadyToConnect(send);
    CHECK(hs.ready);
    CHECK(hs.requests == 1);

    // 2. Setup #1
    std::vector<int> progress;
    const auto result = machine.RunSequence(setupOneAfterHandshake(), send, 3s, {}, [&](int n, int total) { progress.push_back(n * 100 / total); });
    CHECK(result.Ok());
    CHECK(result.repliesExpected == 17);
    CHECK(result.repliesReceived == 17);
    CHECK(!progress.empty() && progress.back() == 100);
    machine.StopReading();

    // 3. Exactly the same writes as in the real session, in the same order:
    //    B6, C1, then the setup block 16 times (once per reply) + once more at the end = 19 writes
    const auto writes = device.Writes();
    const auto recorded = device.RecordedWrites();
    CHECK(device.Mismatches().empty());
    CHECK(writes.size() == 19);
    bool sameAsRecording = writes.size() <= recorded.size();
    for (std::size_t i = 0; sameAsRecording && i < writes.size(); i++)
        sameAsRecording = writes[i] == recorded[i];
    CHECK(sameAsRecording);
    if (!sameAsRecording || !device.Mismatches().empty())
        for (std::size_t i = 0; i < writes.size(); i++)
            std::printf("  write %zu: %s| recorded: %s\n", i, hex(writes[i]).c_str(), i < recorded.size() ? hex(recorded[i]).c_str() : "-");

    const std::vector<std::uint8_t> block = { 0xBA, 0xD6, 0x0A, 0xD5, 0x11, 0x00, 0xCC, 0xCA, 0x81, 0xB7, 0xB0 };
    int blocks = 0;
    for (const auto& w : writes) if (w == block) blocks++;
    CHECK(blocks == 17);
}

static void test_waiting_twice_for_ready_answer_gets_stuck()
{
    // The "stuck at 6 %" bug: the first step of setup #1 waits for the 0xB6 the handshake already took
    RecordedDevice device(g_log);
    if (!device.Loaded()) return;

    LR10KMachine machine;
    machine.StartReading(device, [] { return true; });
    auto send = [&](const std::uint8_t* data, std::size_t size) { std::uint32_t e; return device.Write(data, size, e) == size; };

    CHECK(machine.AskReadyToConnect(send).ready);
    std::vector<int> progress;
    const auto result = machine.RunSequence(LR10KSequences::FirstSetup(), send, 300ms, {}, [&](int n, int total) { progress.push_back(n * 100 / total); });
    machine.StopReading();

    CHECK(result.outcome == LR10KMachine::SequenceOutcome::NoReply);
    CHECK(result.repliesReceived == 0);
    CHECK(result.waitingFor == 0xB6);
    CHECK(progress.size() == 1 && progress[0] == 5); // 1 of 18 (the app rounds this to 6 %)
}

static void test_no_answer_at_all()
{
    // A machine that never answers (switched off / "Remote control" not pressed)
    class SilentPort : public SerialPort
    {
    public:
        bool IsOpen() const override { return true; }
        ReadStatus ReadByte(std::uint8_t&, std::uint32_t& e) override { e = 0; std::this_thread::sleep_for(1ms); return ReadStatus::Timeout; }
        std::size_t Write(const std::uint8_t*, std::size_t size, std::uint32_t& e) override { e = 0; writes++; return size; }
        void DiscardBuffers() override {}
        bool Close() override { return true; }
        std::atomic<int> writes{ 0 };
    } port;

    LR10KMachine machine;
    machine.StartReading(port, [] { return true; });
    auto send = [&](const std::uint8_t* data, std::size_t size) { std::uint32_t e; return port.Write(data, size, e) == size; };
    const auto hs = machine.AskReadyToConnect(send, 31, 5ms, 1ms);
    machine.StopReading();
    CHECK(!hs.ready);
    CHECK(hs.requests == 31);
    CHECK(hs.bytesReceived == 0);
    CHECK(port.writes == 31);
}

static void test_recorded_stream_frames_cleanly()
{
    RecordedDevice device(g_log);
    if (!device.Loaded()) return;
    const auto stream = device.RecordedReplies();
    CHECK(stream.size() > 9000);

    LR10KProtocol::MessageParser parser;
    Message m;
    std::map<int, int> counts;
    int dataPackages = 0, packagesWithHighBytes = 0;
    std::map<std::vector<std::uint8_t>, int> packageCounts;
    std::uint8_t multiplier = 1;
    std::vector<std::uint8_t> statuses;

    for (std::uint8_t b : stream)
    {
        if (!parser.Feed(b, m)) continue;
        counts[(int)m.type]++;
        if (m.type == Message::Type::DataPackage)
        {
            dataPackages++;
            for (int i = 1; i < 6; i++) if (m.data[i] >= 0x80) packagesWithHighBytes++; // would mean lost sync
            packageCounts[std::vector<std::uint8_t>(m.data.begin(), m.data.end())]++;
            g_expectedDistance += LR10KProtocol::DecodeDistanceDelta(m.data[2], m.data[3]);
        }
        else if (m.type == Message::Type::Overflow)
            multiplier = LR10KProtocol::LoadMultiplierFromOverflow(m.Value());
        else if (m.type == Message::Type::Status)
            statuses.push_back(m.Value());
    }

    CHECK(dataPackages == 1591);
    CHECK(packagesWithHighBytes == 0);
    CHECK(counts[(int)Message::Type::Status] == 17);
    CHECK(multiplier == 1); // "B1 03" -> 2^(3-3)

    // The idle live data: EF 4F 00 00 1C 5A, sent ~21 times per second
    const std::vector<std::uint8_t> idle = { 0xEF, 0x4F, 0x00, 0x00, 0x1C, 0x5A };
    CHECK(packageCounts[idle] == 1464);
    CHECK(LR10KProtocol::DecodeLoad(0xEF, 0x4F, 1) == -49);          // raw load counts (zeroed by the app's offset)
    CHECK(LR10KProtocol::DecodeDistanceDelta(0x00, 0x00) == 0);      // not moving

    // Statuses seen: ready (0x00) and "going up" (0x41)
    bool sawReady = false, sawUp = false;
    for (auto s : statuses) { sawReady |= s == 0x00; sawUp |= s == 0x41; }
    CHECK(sawReady && sawUp);
}

static void test_recorded_stream_through_machine()
{
    // The whole recorded byte stream through the machine's reader and interpreter: the readings at the end
    RecordedDevice device(g_log);
    if (!device.Loaded()) return;
    FakeStream port(device.RecordedReplies());

    LR10KMachine machine;
    machine.StartReading(port, [] { return true; });
    machine.StartInterpreter({}, [] { return true; });
    const auto end = std::chrono::steady_clock::now() + 10s;
    while (machine.Readings().dataPackages < 1591 && std::chrono::steady_clock::now() < end)
        std::this_thread::sleep_for(1ms);
    std::this_thread::sleep_for(50ms);
    machine.StopInterpreter();
    machine.StopReading();

    const LR10KReadings r = machine.Readings();
    CHECK(r.dataPackages == 1591);
    CHECK(r.statusMessages == 17);
    CHECK(r.loadMultiplier == 1);
    CHECK(r.load == -49);            // the last package: EF 4F ...
    CHECK(r.hasStatus);
    CHECK(r.distance == g_expectedDistance);
}

static void test_recorded_setup_two_settings()
{
    // The setup #2 message of the session, decoded with the protocol functions
    RecordedDevice device(g_log);
    if (!device.Loaded()) return;
    std::vector<std::uint8_t> setup;
    for (const auto& w : device.RecordedWrites()) if (w.size() > 60) { setup = w; break; }
    CHECK(!setup.empty());
    if (setup.empty()) return;

    auto valueAfter = [&](std::vector<std::uint8_t> tag) -> float {
        for (std::size_t i = 0; i + tag.size() + 8 <= setup.size(); i++)
            if (std::equal(tag.begin(), tag.end(), setup.begin() + i))
                return LR10KProtocol::DecodeFloat(&setup[i + tag.size()]);
        return -1.0f;
    };
    CHECK(valueAfter({ 0xB6, 0xD1, 0x07 }) == 250.0f);          // fixed value at the start
    CHECK(valueAfter({ 0xD3, 0x07, 0x00, 0xD1, 0x02 }) == 30.0f);  // test speed 1: 30 mm/min
    CHECK(valueAfter({ 0xD3, 0x02, 0x02, 0xD1, 0x06 }) == 161.0f); // jog speed: 161 mm/min
    CHECK(valueAfter({ 0xD3, 0x04, 0x00, 0xD1, 0x05 }) == 750.0f); // extension limit (sent 3 times)
    CHECK(valueAfter({ 0xD5, 0x09, 0x00, 0xD1, 0x00 }) == 10000.0f); // load capacity 10 kN
}

int main(int argc, char** argv)
{
    if (argc > 1) g_log = argv[1];
    if (!std::filesystem::exists(g_log))
    {
        std::printf("connect_replay_test: log not found: %s (run from the repository root)\n", g_log.string().c_str());
        return EXIT_FAILURE;
    }

    test_connect_matches_real_session();
    test_waiting_twice_for_ready_answer_gets_stuck();
    test_no_answer_at_all();
    test_recorded_stream_frames_cleanly();
    test_recorded_stream_through_machine();
    test_recorded_setup_two_settings();

    if (g_failures == 0)
        std::printf("connect_replay_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
