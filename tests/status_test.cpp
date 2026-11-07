// Unit tests for LR10KStatus (main/LloydLR10KTensile/LR10KStatus.h): connection state and status texts.
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I main/LloydLR10KTensile tests/status_test.cpp -o status_test
//   ./status_test
//
// Run automatically by .github/workflows/tests.yml.

#include "LR10KStatus.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)
#define SAME_TEXT(a, b) CHECK(std::strcmp((a), (b)) == 0)

static void test_start_state()
{
    LR10KStatus s;
    const auto snap = s.Get();
    CHECK(!snap.PortOpen());
    CHECK(snap.comPort == -1 && snap.lastComPort == -1);
    CHECK(!snap.machineConnected && !snap.liveData && !snap.zeroPointSet);
    SAME_TEXT(snap.status, "N/A");
    SAME_TEXT(snap.warnError, "N/A");
    SAME_TEXT(snap.LiveText(), "N/A");
}

static void test_connect_and_disconnect()
{
    LR10KStatus s;

    s.OnPortOpened(4);
    CHECK(s.IsPortOpen());
    CHECK(s.ComPort() == 4 && s.LastComPort() == 4);
    CHECK(!s.IsMachineConnected());

    s.OnMachineConnected();
    s.SetStatus("Applying settings ...");
    auto snap = s.Get();
    CHECK(snap.machineConnected);
    SAME_TEXT(snap.LiveText(), "No");
    SAME_TEXT(snap.warnError, "OK");
    SAME_TEXT(snap.status, "Applying settings ...");

    s.SetLiveData(true);
    SAME_TEXT(s.Get().LiveText(), "Yes");
    s.SetWarnError("Load cell overload");
    s.SetStatus("Ready");
    s.SetZeroPointSet(true);

    // Disconnecting from the LR 10K: everything that belongs to the connection goes back to N/A at once
    s.OnMachineDisconnected();
    snap = s.Get();
    CHECK(!snap.machineConnected && !snap.liveData);
    SAME_TEXT(snap.LiveText(), "N/A");
    SAME_TEXT(snap.status, "N/A");
    SAME_TEXT(snap.warnError, "N/A");
    CHECK(snap.PortOpen()); // the COM port is closed separately
    CHECK(snap.zeroPointSet); // (unchanged, as before)

    // Closing the port keeps the last port for "Reconnect"
    s.OnPortClosed();
    CHECK(!s.IsPortOpen());
    CHECK(s.ComPort() == -1);
    CHECK(s.LastComPort() == 4);

    s.OnPortOpened(7);
    CHECK(s.LastComPort() == 7);
}

static void test_live_text_needs_a_connection()
{
    LR10KStatus s;
    s.SetLiveData(true); // e.g. demo mode or a late call: still N/A while not connected
    SAME_TEXT(s.Get().LiveText(), "N/A");
}

static void test_null_text_is_ignored()
{
    LR10KStatus s;
    s.SetStatus("Ready");
    s.SetStatus(nullptr); // e.g. LR10KProtocol::StatusText() for an unknown code
    SAME_TEXT(s.Status(), "Ready");
    s.SetWarnError(nullptr);
    SAME_TEXT(s.WarnError(), "N/A");
}

// A worker thread connects/disconnects while the "GUI" reads snapshots (ThreadSanitizer checks the access).
// Every snapshot must be a whole state: connected with warning "OK", or disconnected with everything N/A.
static void test_threads_see_whole_states()
{
    LR10KStatus s;
    s.OnPortOpened(3);
    std::atomic<bool> stop{ false };
    std::atomic<int> torn{ 0 };

    std::thread worker([&] {
        for (int i = 0; i < 20000; ++i)
        {
            if (i % 2 == 0) s.OnMachineConnected();
            else s.OnMachineDisconnected();
        }
        stop.store(true);
    });

    long reads = 0;
    while (!stop.load())
    {
        const auto snap = s.Get();
        const bool connectedState = snap.machineConnected && std::strcmp(snap.warnError, "OK") == 0 && !snap.liveData;
        const bool disconnectedState = !snap.machineConnected && std::strcmp(snap.warnError, "N/A") == 0
            && std::strcmp(snap.status, "N/A") == 0 && !snap.liveData;
        if (!connectedState && !disconnectedState && reads > 0)
            torn.fetch_add(1);
        ++reads;
    }
    worker.join();
    CHECK(torn.load() == 0);
}

int main()
{
    test_start_state();
    test_connect_and_disconnect();
    test_live_text_needs_a_connection();
    test_null_text_is_ignored();
    test_threads_see_whole_states();

    if (g_failures == 0)
        std::printf("status_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
