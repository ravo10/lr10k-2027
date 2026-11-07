/**

    @file      LR10KBufferMessage.cpp
    @brief     Contains the logic for building up a buffer message that is to be sent to the LR 10K Tensile Machine.
                And also logic for finding a float value from binary data. (from the LR 10K)
    @details   ~
    @author    Rasmus Ullestad Relling, (original logic is from other teachers from USN)
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/



#include "LR10KBufferMessage.h"
#include "LR10KProtocol.h" //!< Float encoding (unit tested in tests/protocol_test.cpp)

LR10KBufferMessage::LR10KBufferMessage()
{
}

LR10KBufferMessage::~LR10KBufferMessage()
{
}

// Private
void LR10KBufferMessage::clearBufferSystem()
{
    for (size_t i = 0; i < sizeof(message); i++) message[i] = { 0 };
    ResetSizeOfMessage();
}

// Public
void LR10KBufferMessage::ResetSizeOfMessage()
{
    size_of_message = { 0 };
}

float LR10KBufferMessage::FindFloat(byte* pbTable)
{
    return LR10KProtocol::DecodeFloat(pbTable);
}

void LR10KBufferMessage::DataBuildUpBufferMessageFromFloat(float f)
{
    // Set a flag to indicate that this is not a single buffer message
    singleBufferMessage = false;

    // The float as 8 ASCII hex characters, most significant first
    const std::array<std::uint8_t, 8> chars = LR10KProtocol::EncodeFloat(f);
    for (size_t i = 0; i < chars.size(); i++)
        message[size_of_message + i] = chars[i];

    // Update the size of the message buffer
    size_of_message += chars.size();
}

std::vector<std::pair<byte, bool>> LR10KBufferMessage::GetVectorFromFloat(float f)
{
    // The float as 8 ASCII hex characters, most significant first (all "write" bytes)
    std::vector<std::pair<byte, bool>> HEX_sequence = {  };
    for (std::uint8_t c : LR10KProtocol::EncodeFloat(f))
        HEX_sequence.push_back(std::make_pair(c, false));

    return HEX_sequence;
}

void LR10KBufferMessage::DataBuildUpBufferMessage(std::vector<byte> hexdecimalData, bool clearBuffer)
{
    singleBufferMessage = false;

    if (clearBuffer)
        clearBufferSystem();

    for (size_t i = 0; i < hexdecimalData.size(); i++)
    {
        message[size_of_message++] = hexdecimalData[i];
    }
}

void LR10KBufferMessage::DataSetSingleBufferMessage(byte hexdecimalData)
{
    singleBufferMessage = true;

    message_single = hexdecimalData;
}

bool LR10KBufferMessage::isBuffSingle()
{
    return singleBufferMessage;
}
