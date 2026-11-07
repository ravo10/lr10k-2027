// Unit tests for the LR 10K protocol functions (main/LloydLR10KTensile/LR10KProtocol.h).
//
//   g++ -std=c++17 -Wall -Wextra -I main/LloydLR10KTensile tests/protocol_test.cpp main/LloydLR10KTensile/LR10KBufferMessage.cpp -o protocol_test
//   ./protocol_test
//
// Run automatically by .github/workflows/tests.yml.
//
// Two kinds of checks:
//  1. Known values from the sniffed protocol (the setup messages in LR10KController.h).
//  2. The new functions behave exactly like the original decoding code for EVERY input. The
//     original functions are copied below (only Windows types replaced) as "legacy" references.

#include "LR10KProtocol.h"
#include "LR10KBufferMessage.h" // the app class that builds setup messages

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace LR10KProtocol;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failures; } } while (0)

// -----------------------------------------------------------------------------
// Legacy code, copied from LR10KDecryption.cpp / LR10KBufferMessage.cpp before the move.
// Only changes: __int16 -> int16_t, LPARAM -> long, byte -> uint8_t, the global multiplier
// "bMultiplier_load" is a parameter, and the input/output buffers are local.
// -----------------------------------------------------------------------------
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits" // legacy: "uData1 >= 0xF00" can never be true for a byte (both branches are equal anyway)
#endif
namespace legacy
{
    typedef std::uint8_t byte;

    inline long map(long x, long in_min, long in_max, long out_min, long out_max)
    {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    std::int16_t DecodingBits_Load(byte uData1, byte uData2, byte bMultiplier_load)
    {
        std::int16_t lParam{ 0 };
        lParam = uData1;
        lParam &= 0x1F;  // Mask away the first 3 bits
        lParam <<= 7;
        lParam += uData2;

        if (bMultiplier_load > 1)
        {
            if (lParam >= 0x0C00)
                lParam = (short)map(lParam, 0x0C00, 0x0FFF, 0x800 * bMultiplier_load / 2, 0x800 * bMultiplier_load);
            else
                lParam = (short)map(lParam, 0x03FF, 0x0, -2048 * bMultiplier_load / 2, -2048 * bMultiplier_load);
        }
        else
        {
            if (uData1 >= 0xF00)  // Upper half
                lParam -= 0x800;
            else
                lParam = -0x800 + lParam;
        }

        return (long)lParam;
    }

    std::int16_t DecodingBits_Distance(byte uData1, byte uData2)
    {
        std::int16_t lParam{ 0 };
        lParam = uData1;
        lParam <<= 9;
        short tmp16 = uData2 << 2;
        lParam += tmp16;

        return (long)lParam;
    }

    void DecodingBits_Extensometer(byte uData1, byte uData2, std::int16_t* lParam)
    {
        *lParam = uData1 << 7;
        *lParam += uData2;
    }

    byte OverflowMultiplier(byte lParam)
    {
        return (byte)pow(2.0, (double)(3.0 - (byte)lParam));
    }

    union UValue { float f; byte b[4]; };

    void GetVectorFromFloat(float f, byte out[8])
    {
        byte* data = out;
        UValue value;
        value.f = f;
        data[0] = value.b[3] >> 4;
        data[1] = value.b[3] & 0x0F;
        data[2] = value.b[2] >> 4;
        data[3] = value.b[2] & 0x0F;
        data[4] = value.b[1] >> 4;
        data[5] = value.b[1] & 0x0F;
        data[6] = value.b[0] >> 4;
        data[7] = value.b[0] & 0x0F;
        for (int i = 0; i < 8; i++)
        {
            if (data[i] <= 9)
                data[i] += '0';
            else
            {
                data[i] -= 10;
                data[i] += 'A';
            }
        }
    }

    float FindFloat(const byte* pbTable)
    {
        UValue value;
        byte data[8] = { 0 };
        for (int i = 0; i < 8; i++)
        {
            if (pbTable[i] >= '0' && pbTable[i] <= '9')
                data[i] = pbTable[i] - '0';
            else if (pbTable[i] >= 'A' && pbTable[i] <= 'F')
                data[i] = pbTable[i] - 'A' + 10;
            else
                data[i] = 0;
        }
        value.b[3] = (data[0] << 4) | data[1];
        value.b[2] = (data[2] << 4) | data[3];
        value.b[1] = (data[4] << 4) | data[5];
        value.b[0] = (data[6] << 4) | data[7];
        return value.f;
    }
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// -----------------------------------------------------------------------------

static bool sameBits(float a, float b) { return std::memcmp(&a, &b, sizeof(float)) == 0; }

static void test_known_setup_values()
{
    // From Write_SecondSetup_Begin: 0x34 0x33 0x37 0x41 0x30 0x30 0x30 0x30 = "437A0000" = 250.0f
    const std::uint8_t setupBegin[8] = { 0x34, 0x33, 0x37, 0x41, 0x30, 0x30, 0x30, 0x30 };
    CHECK(DecodeFloat(setupBegin) == 250.0f);
    // From Write_SecondSetup_End4_3: "461C4000" = 10000.0f (the 10 kN load capacity)
    const std::uint8_t setupEnd[8] = { 0x34, 0x36, 0x31, 0x43, 0x34, 0x30, 0x30, 0x30 };
    CHECK(DecodeFloat(setupEnd) == 10000.0f);

    // Default test speed 100 mm/min is sent as "42C80000"
    const auto speed = EncodeFloat(100.0f);
    CHECK(std::memcmp(speed.data(), "42C80000", 8) == 0);
    const auto limit = EncodeFloat(10000.0f);
    CHECK(std::memcmp(limit.data(), setupEnd, 8) == 0);
    const auto negative = EncodeFloat(-1.5f); // 0xBFC00000
    CHECK(std::memcmp(negative.data(), "BFC00000", 8) == 0);
}

static void test_known_decoding()
{
    // Load, multiplier 1: 12-bit value with 0x800 as zero; the top 3 bits of byte 1 are the package marker
    CHECK(DecodeLoad(0xF0, 0x00, 1) == 0);      // 0x10 << 7 = 0x800 -> 0
    CHECK(DecodeLoad(0xF0, 0x01, 1) == 1);
    CHECK(DecodeLoad(0xEF, 0x7F, 1) == -1);     // 0x0F << 7 | 0x7F = 0x7FF -> -1
    CHECK(DecodeLoad(0x10, 0x00, 1) == DecodeLoad(0xF0, 0x00, 1)); // marker bits ignored

    // Load, multiplier 2: 0xC00..0xFFF -> +2048..+4096, 0x3FF..0x000 -> -2048..-4096
    CHECK(DecodeLoad(0x18, 0x00, 2) == 2048);   // 0xC00
    CHECK(DecodeLoad(0x1F, 0x7F, 2) == 4096);   // 0xFFF
    CHECK(DecodeLoad(0x07, 0x7F, 2) == -2048);  // 0x3FF
    CHECK(DecodeLoad(0x00, 0x00, 2) == -4096);  // 0x000

    // Distance delta: signed through 16-bit wrap-around
    CHECK(DecodeDistanceDelta(0x00, 0x01) == 4);
    CHECK(DecodeDistanceDelta(0x01, 0x00) == 512);
    CHECK(DecodeDistanceDelta(0x7F, 0x7F) == -4);
    CHECK(DecodeDistanceDelta(0x00, 0x00) == 0);

    // Overflow message -> load multiplier
    CHECK(LoadMultiplierFromOverflow(0) == 8);
    CHECK(LoadMultiplierFromOverflow(1) == 4);
    CHECK(LoadMultiplierFromOverflow(2) == 2);
    CHECK(LoadMultiplierFromOverflow(3) == 1);
    CHECK(LoadMultiplierFromOverflow(4) == 0);

    CHECK(DecodeExtensometer(0x01, 0x02) == 130);
}

static void test_identical_to_legacy_for_all_inputs()
{
    long mismatches = 0;
    const std::uint8_t multipliers[] = { 0, 1, 2, 4, 8 };

    for (int d1 = 0; d1 < 256; d1++)
    {
        for (int d2 = 0; d2 < 256; d2++)
        {
            const std::uint8_t a = (std::uint8_t)d1, b = (std::uint8_t)d2;
            for (std::uint8_t m : multipliers)
                if (DecodeLoad(a, b, m) != legacy::DecodingBits_Load(a, b, m)) mismatches++;

            if (DecodeDistanceDelta(a, b) != legacy::DecodingBits_Distance(a, b)) mismatches++;

            std::int16_t ext = 0;
            legacy::DecodingBits_Extensometer(a, b, &ext);
            if (DecodeExtensometer(a, b) != ext) mismatches++;
        }
        if (LoadMultiplierFromOverflow((std::uint8_t)d1) != legacy::OverflowMultiplier((std::uint8_t)d1)) mismatches++;
    }
    CHECK(mismatches == 0);
    if (mismatches) std::printf("  %ld decoding mismatches against the original code\n", mismatches);
}

static void test_float_encoding_identical_to_legacy()
{
    long mismatches = 0;
    // Many float bit patterns, including typical setup values, 0, negatives, tiny and huge numbers
    const float samples[] = { 0.0f, -0.0f, 1.0f, 0.5f, 100.0f, 250.0f, 10000.0f, 500.0f, 1000.0f, 0.001f,
                              1e-30f, 3.4e38f, -42.25f, 12.345f, 0.1f };
    for (float f : samples)
    {
        std::uint8_t old[8];
        legacy::GetVectorFromFloat(f, old);
        const auto now = EncodeFloat(f);
        if (std::memcmp(old, now.data(), 8) != 0) mismatches++;
        if (!sameBits(DecodeFloat(now.data()), f)) mismatches++;
        if (!sameBits(legacy::FindFloat(now.data()), DecodeFloat(now.data()))) mismatches++;
    }
    for (std::uint32_t bits = 0; bits < 0xFFFFFFF0u; bits += 0x00F00F01u) // ~270 spread-out bit patterns
    {
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        std::uint8_t old[8];
        legacy::GetVectorFromFloat(f, old);
        const auto now = EncodeFloat(f);
        if (std::memcmp(old, now.data(), 8) != 0) mismatches++;
        if (!sameBits(DecodeFloat(now.data()), f)) mismatches++;
    }
    // Invalid characters count as 0 in both
    const std::uint8_t junk[8] = { 'x', '4', '3', 'z', '7', 'A', '0', '-' };
    if (!sameBits(legacy::FindFloat(junk), DecodeFloat(junk))) mismatches++;

    CHECK(mismatches == 0);
    if (mismatches) std::printf("  %ld float encoding mismatches against the original code\n", mismatches);
}

static void test_app_buffer_message_class()
{
    // The class the app uses to build setup messages must produce the same bytes as before
    LR10KBufferMessage message;
    for (float f : { 100.0f, 250.0f, 10000.0f, 12.5f })
    {
        std::uint8_t old[8];
        legacy::GetVectorFromFloat(f, old);

        const auto sequence = message.GetVectorFromFloat(f);
        CHECK(sequence.size() == 8);
        bool same = sequence.size() == 8;
        for (std::size_t i = 0; i < sequence.size() && same; i++)
            same = sequence[i].first == old[i] && sequence[i].second == false; // all "write" bytes
        CHECK(same);
    }

    LR10KBufferMessage built;
    built.DataBuildUpBufferMessage({ 0xD3, 0x07 });
    built.DataBuildUpBufferMessageFromFloat(250.0f);
    CHECK(built.size_of_message == 10);
    CHECK(built.message[0] == 0xD3 && built.message[1] == 0x07);
    CHECK(std::memcmp(&built.message[2], "437A0000", 8) == 0);
    CHECK(built.FindFloat(&built.message[2]) == 250.0f);
}

int main()
{
    test_known_setup_values();
    test_known_decoding();
    test_identical_to_legacy_for_all_inputs();
    test_float_encoding_identical_to_legacy();
    test_app_buffer_message_class();

    if (g_failures == 0)
        std::printf("protocol_test: all tests passed\n");
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
