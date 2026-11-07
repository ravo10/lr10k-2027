/**

    @file      LR10KSequences.h
    @brief     Sniffed send/expect sequences for the LR 10K, shared by the app and the tests.
    @details   Each step is (byte, expectReply): false = add the byte to the message being built,
               true = send the message built so far and wait until the LR 10K sends this byte
               (see LR10KMachine::RunSequence). Moved unchanged from LR10KController.h.

**/
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace LR10KSequences
{
    /**
        @brief  Setup #1: makes the LR 10K "Ready" (yellow light). Needed again after it was disconnected.
        @details The first step (wait for 0xB6) is the answer to the "ready to connect" request
                 (LR10KMachine::AskReadyToConnect), which is sent before this sequence.
                 Another thing noticed: the last byte written is sent back by the LR 10K until it
                 responds to a new byte. E.g. 0x00 & 0xB6 will be sent out until 0xB7 is sent and the
                 new response is 0xBA.
    **/
    inline const std::vector<std::pair<std::uint8_t, bool>>& FirstSetup()
    {
        static const std::vector<std::pair<std::uint8_t, bool>> sequence = {
        /* { 0xB6, false },  //!< SEND STATUS */ // This one can not be enabled.. Only for documentation. "AskAndWaitForLR10KForReadyToConnect" function will send this byte.
        //    { 0xB0, true },
        //{ 0xB6, false },  //!< SEND STATUS
        //   { 0x0B, true },
        //{ 0xB6, false },  //!< SEND STATUS
            { 0xB6, true },
        { 0xC1, false },
            { 0x00, true },
        { 0xBA, false },
        { 0xD6, false },
        { 0x0A, false },
        { 0xD5, false },
        { 0x11, false },
        { 0x00, false },
        { 0xCC, false },
        { 0xCA, false },
        { 0x81, false },
        { 0xB7, false },
        { 0xB0, false },
            { 0xBA, true },
            // This is the last things the LR 10K sends out
            { 0xB2, true },
            { 0x00, true },
            { 0xB3, true },
            { 0x20, true },
            { 0xB1, true },
            { 0x03, true },
            { 0xB7, true },
            { 0x33, true },
            { 0x39, true },
            { 0x38, true },
            { 0x36, true },
            { 0x44, true },
            { 0x31, true },
            { 0x34, true },
            { 0x32, true } // Could only check this, but for documentation purposes and loading overview..
    };
        return sequence;
    }
}
