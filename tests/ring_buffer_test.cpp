// Unit tests for CRingBuffer (main/LloydLR10KTensile/RingBuffer.h).
// Portable: builds with g++/clang++ on Linux or MSVC on Windows.
//
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I main/LloydLR10KTensile tests/ring_buffer_test.cpp -o ring_buffer_test
//   ./ring_buffer_test
//
// Run automatically by .github/workflows/tests.yml.

#include "RingBuffer.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

static void test_fifo_order()
{
    CRingBuffer rb;
    for (int i = 0; i < 10; ++i)
        CHECK(rb.TryWrite(static_cast<std::uint8_t>(i)));
    CHECK(rb.Size() == 10);
    for (int i = 0; i < 10; ++i)
    {
        std::uint8_t b = 0xFF;
        CHECK(rb.TryRead(b, 0ms));
        CHECK(b == i);
    }
    CHECK(!rb.DataAvailable());
}

static void test_full_and_wraparound()
{
    CRingBuffer rb;
    for (std::size_t i = 0; i < CRingBuffer::bufferSize; ++i)
        CHECK(rb.TryWrite(static_cast<std::uint8_t>(i)));
    CHECK(!rb.TryWrite(0xAA));            // full: dropped, not overwritten
    CHECK(!rb.WaitWrite(0xAA, 10ms));     // still full after waiting

    // Read half, write more to force the indices to wrap.
    for (std::size_t i = 0; i < CRingBuffer::bufferSize / 2; ++i)
    {
        std::uint8_t b = 0;
        CHECK(rb.TryRead(b, 0ms));
        CHECK(b == static_cast<std::uint8_t>(i));
    }
    for (std::size_t i = 0; i < CRingBuffer::bufferSize / 2; ++i)
        CHECK(rb.TryWrite(static_cast<std::uint8_t>(1000 + i)));

    for (std::size_t i = CRingBuffer::bufferSize / 2; i < CRingBuffer::bufferSize; ++i)
    {
        std::uint8_t b = 0;
        CHECK(rb.TryRead(b, 0ms));
        CHECK(b == static_cast<std::uint8_t>(i));
    }
    for (std::size_t i = 0; i < CRingBuffer::bufferSize / 2; ++i)
    {
        std::uint8_t b = 0;
        CHECK(rb.TryRead(b, 0ms));
        CHECK(b == static_cast<std::uint8_t>(1000 + i));
    }
    CHECK(rb.Size() == 0);
}

static void test_read_timeout_returns_no_stale_data()
{
    CRingBuffer rb;
    CHECK(rb.TryWrite(0x42));
    std::uint8_t b = 0;
    CHECK(rb.TryRead(b, 0ms));
    CHECK(b == 0x42);

    // The old buffer returned a stale byte here and corrupted its indices.
    b = 0x99;
    const auto t0 = std::chrono::steady_clock::now();
    CHECK(!rb.TryRead(b, 30ms));
    const auto waited = std::chrono::steady_clock::now() - t0;
    CHECK(b == 0x99);                     // untouched on timeout
    CHECK(waited >= 25ms);
    CHECK(rb.Size() == 0);

    CHECK(rb.TryWrite(0x43));             // still consistent afterwards
    CHECK(rb.TryRead(b, 0ms));
    CHECK(b == 0x43);
}

static void test_clear()
{
    CRingBuffer rb;
    for (int i = 0; i < 50; ++i)
        rb.TryWrite(static_cast<std::uint8_t>(i));
    rb.ClearAllData();
    CHECK(rb.Size() == 0);
    std::uint8_t b = 0;
    CHECK(!rb.TryRead(b, 0ms));
    CHECK(rb.TryWrite(7));
    CHECK(rb.TryRead(b, 0ms));
    CHECK(b == 7);
}

static void test_reader_wakes_on_write()
{
    CRingBuffer rb;
    std::atomic<bool> got{ false };
    std::uint8_t value = 0;
    std::thread reader([&] {
        got = rb.TryRead(value, 2000ms);
    });
    std::this_thread::sleep_for(20ms);
    rb.TryWrite(0x5A);
    reader.join();
    CHECK(got);
    CHECK(value == 0x5A);
}

static void test_producer_consumer_threads()
{
    // Mirrors the app: one thread reading the COM port into the buffer, another
    // thread decoding. Every byte must arrive exactly once, in order.
    CRingBuffer rb;
    constexpr int total = 200000;
    std::thread producer([&] {
        for (int i = 0; i < total; ++i)
            while (!rb.WaitWrite(static_cast<std::uint8_t>(i & 0xFF), 50ms)) {}
    });

    int received = 0;
    bool inOrder = true;
    while (received < total)
    {
        std::uint8_t b = 0;
        if (!rb.TryRead(b, 1000ms))
            break;
        if (b != static_cast<std::uint8_t>(received & 0xFF))
            inOrder = false;
        ++received;
    }
    producer.join();
    CHECK(received == total);
    CHECK(inOrder);
    CHECK(rb.Size() == 0);
}

int main()
{
    test_fifo_order();
    test_full_and_wraparound();
    test_read_timeout_returns_no_stale_data();
    test_clear();
    test_reader_wakes_on_write();
    test_producer_consumer_threads();

    if (g_failures == 0)
        std::printf("ring_buffer_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
