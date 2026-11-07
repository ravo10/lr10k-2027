/**

    @file      LR10KProtocol.h
    @brief     The reverse-engineered LR 10K serial protocol: decoding of the data the machine sends
               and encoding of setup values sent to it.
    @details   Pure functions with no Windows, GUI or global state, so they can be unit tested
               (tests/protocol_test.cpp) and ported as-is (e.g. to Rust). The behaviour is exactly
               that of the original decoding code (LR10KDecryption.cpp / LR10KBufferMessage.cpp),
               which the tests check for every possible input.

               Messages from the LR 10K (first byte):
                 >= 0xE0  data package: 6 bytes = load (2), distance delta (2), extensometer (2)
                 0xB1     overflow/range message: 1 more byte -> load multiplier
                 0xB6     status message: 1 more byte (0x00 ready, 0x41 up, 0x61 down, ...)
                 0xBF     warning/error message: 1 more byte

**/
#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace LR10KProtocol
{
    constexpr std::uint8_t DataPackageMin     = 0xE0; //!< First byte >= this: a data package follows.
    constexpr std::uint8_t MsgOverflow        = 0xB1;
    constexpr std::uint8_t MsgStatus          = 0xB6;
    constexpr std::uint8_t MsgWarnError       = 0xBF;

    /**
        @brief  Linear re-mapping with integer arithmetic (truncates toward zero), like Arduino's map().
    **/
    inline long long Map(long long x, long long inMin, long long inMax, long long outMin, long long outMax)
    {
        return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
    }

    /**
        @brief  Load multiplier from the byte after an overflow message (0xB1): 2^(3 - value).
        @details 0 -> 8, 1 -> 4, 2 -> 2, 3 -> 1, larger values -> 0 (treated like 1 by DecodeLoad).
    **/
    inline std::uint8_t LoadMultiplierFromOverflow(std::uint8_t value)
    {
        return (std::uint8_t)std::pow(2.0, 3.0 - (double)value);
    }

    /**
        @brief  Raw load (counts) from the first two bytes of a data package.
        @param  data1      - First byte (only the low 5 bits are load data).
        @param  data2      - Second byte.
        @param  multiplier - Current load multiplier (see LoadMultiplierFromOverflow), used above ~1600 N.
        @retval Signed raw load. Multiply by LR10KAppGlobals::Convert_to_Newton for newtons.
    **/
    inline std::int16_t DecodeLoad(std::uint8_t data1, std::uint8_t data2, std::uint8_t multiplier)
    {
        int value = ((data1 & 0x1F) << 7) + data2; // 12-bit value, 0x800 = zero

        if (multiplier > 1)
        {
            if (value >= 0x0C00)
                value = (std::int16_t)Map(value, 0x0C00, 0x0FFF, 0x800 * multiplier / 2, 0x800 * multiplier);
            else
                value = (std::int16_t)Map(value, 0x03FF, 0x0, -2048 * multiplier / 2, -2048 * multiplier);
        }
        else
            value -= 0x800;

        return (std::int16_t)value;
    }

    /**
        @brief  Distance change (counts) since the previous data package, from bytes 3 and 4.
        @details Signed through 16-bit wrap-around (e.g. 0x7F 0x7F = -4). The app adds these up.
                 Multiply the sum by LR10KAppGlobals::Convert_to_mm for millimetres.
    **/
    inline std::int16_t DecodeDistanceDelta(std::uint8_t data1, std::uint8_t data2)
    {
        return (std::int16_t)(std::uint16_t)((data1 << 9) + (data2 << 2));
    }

    /**
        @brief  Raw extensometer value from bytes 5 and 6 (meaning not fully known yet).
    **/
    inline std::int16_t DecodeExtensometer(std::uint8_t data1, std::uint8_t data2)
    {
        return (std::int16_t)(std::uint16_t)((data1 << 7) + data2);
    }

    /**
        @brief  Encode a float as the LR 10K expects in setup messages: its IEEE-754 bits as
                8 ASCII hex characters, most significant first (e.g. 250.0f -> "437A0000").
    **/
    inline std::array<std::uint8_t, 8> EncodeFloat(float value)
    {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));

        std::array<std::uint8_t, 8> out{};
        for (int i = 0; i < 8; i++)
        {
            const std::uint8_t nibble = (std::uint8_t)((bits >> (28 - 4 * i)) & 0x0F);
            out[i] = (std::uint8_t)(nibble <= 9 ? '0' + nibble : 'A' + (nibble - 10));
        }
        return out;
    }

    /**
        @brief  Decode 8 ASCII hex characters (see EncodeFloat) back to a float. Invalid characters count as 0.
    **/
    inline float DecodeFloat(const std::uint8_t* chars)
    {
        std::uint32_t bits = 0;
        for (int i = 0; i < 8; i++)
        {
            std::uint8_t nibble = 0;
            if (chars[i] >= '0' && chars[i] <= '9')      nibble = (std::uint8_t)(chars[i] - '0');
            else if (chars[i] >= 'A' && chars[i] <= 'F') nibble = (std::uint8_t)(chars[i] - 'A' + 10);
            bits = (bits << 4) | nibble;
        }
        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
}
