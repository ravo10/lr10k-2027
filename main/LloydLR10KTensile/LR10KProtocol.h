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

    // -------------------------------------------------------------------------
    // Status and warning/error codes
    // -------------------------------------------------------------------------

    /**
        @brief  Text for the value of a status message (0xB6 + value), or nullptr if the code is not known.
    **/
    inline const char* StatusText(std::uint8_t code)
    {
        switch (code)
        {
        case 0x00: return "Ready";
        case 0x40: return "Stopped fast movement";
        case 0x41: return "Going up";
        case 0x61: return "Going down";
        default:   return nullptr;
        }
    }

    /**
        @brief  Text for the value of a warning/error message (0xBF + value), or nullptr if the code is not known.
        @details Warnings stay until the user ends a test or sends a new setup.
    **/
    inline const char* WarnErrorText(std::uint8_t code)
    {
        switch (code)
        {
        case 0x00: return "OK";
        case 0x02: return "Tension overload stop. Go DOWN only";
        case 0x05: return "Lower crosshead limit stop. Go UP only";
        case 0x06: return "Compression overload stop. Go UP only";
        case 0x0C: return "Extension range exceeded (error 12)";
        case 0x12: return "Keyboard buffer overflow";
        default:   return nullptr;
        }
    }

    // -------------------------------------------------------------------------
    // Message framing
    // -------------------------------------------------------------------------

    //! One complete message from the LR 10K.
    struct Message
    {
        enum class Type : std::uint8_t
        {
            DataPackage, //!< 6 bytes: load (data[0..1]), distance delta (data[2..3]), extensometer (data[4..5])
            Overflow,    //!< 0xB1 + value: load multiplier (see LoadMultiplierFromOverflow)
            Status,      //!< 0xB6 + value: 0x00 ready, 0x40 stopped fast movement, 0x41 up, 0x61 down, ...
            WarnError,   //!< 0xBF + value: warning/error (e.g. magnetic stop)
            Unknown      //!< Any other single byte (data[0])
        };

        Type type = Type::Unknown;
        std::array<std::uint8_t, 6> data{}; //!< The message bytes, first byte at data[0]

        //! The value byte of an Overflow/Status/WarnError message.
        std::uint8_t Value() const { return data[1]; }
    };

    /**
        @brief  Splits the byte stream from the LR 10K into complete messages.
        @details Pure state machine: feed bytes one at a time, in any chunks. Frames messages exactly like
                 the original interpreter loop (first byte >= 0xE0: 6-byte data package; 0xB1/0xB6/0xBF: 2 bytes;
                 anything else: a 1-byte unknown message). Tested in tests/protocol_test.cpp.
    **/
    class MessageParser
    {
    public:
        /**
            @brief  Add the next byte.
            @retval true when this byte completed a message, which is written to @p out.
        **/
        bool Feed(std::uint8_t byte, Message& out)
        {
            if (m_expected == 0)
            {
                // First byte of a new message
                m_current = Message{};
                m_current.data[0] = byte;
                m_received = 1;

                if (byte >= DataPackageMin)
                    return Begin(Message::Type::DataPackage, 6);
                switch (byte)
                {
                case MsgOverflow:  return Begin(Message::Type::Overflow, 2);
                case MsgStatus:    return Begin(Message::Type::Status, 2);
                case MsgWarnError: return Begin(Message::Type::WarnError, 2);
                default:
                    m_current.type = Message::Type::Unknown;
                    out = m_current;
                    m_received = 0;
                    return true;
                }
            }

            m_current.data[m_received++] = byte;
            if (m_received < m_expected)
                return false;

            out = m_current;
            m_expected = 0;
            m_received = 0;
            return true;
        }

        //! Forget a partly received message (e.g. after reconnecting).
        void Reset() { m_expected = 0; m_received = 0; }

        //! True while in the middle of a message.
        bool InMessage() const { return m_expected != 0; }

    private:
        bool Begin(Message::Type type, int length)
        {
            m_current.type = type;
            m_expected = length;
            return false;
        }

        Message m_current{};
        int m_expected = 0; //!< Length of the message being received, 0 = waiting for a first byte
        int m_received = 0;
    };

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
