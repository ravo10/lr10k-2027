/**

    @file      RingBuffer.cpp
    @brief     Blocking Read()/Write() for CRingBuffer that honour the LR 10K thread stop flags.
    @details   The buffer itself is in RingBuffer.h. These wrappers wait in short slices so a
               stopped thread notices the stop flag within ~20 ms instead of spinning.
    @author    Rasmus Ullestad Relling (orginal logic by Olaf Hallan Graven)
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/


#include "LR10KDecryption.h"

namespace {
    constexpr std::chrono::milliseconds waitSlice{ 20 };
}

bool CRingBuffer::Write(std::uint8_t Element, bool NonBlocking /*= false*/)
{
    if (NonBlocking)
        return TryWrite(Element);

    // Wait for space while the COM reader is allowed to run
    const auto start = std::chrono::steady_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(3, start))
    {
        if (WaitWrite(Element, waitSlice))
            return true;
    }

    return false;
}

std::uint8_t CRingBuffer::Read(bool configDone)
{
    std::uint8_t value = 0x00; // = 0x00 when no data

    const auto start = std::chrono::steady_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(4, start, configDone))
    {
        if (TryRead(value, waitSlice))
            return value;
    }

    return 0x00;
}
