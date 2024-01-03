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
    UValue value;
    byte data[8] = { 0 };

    // Step 1: Loop through each character in the input
    for (int i = 0; i < 8; i++)
    {
        // Step 2: Check if the character is a valid hexadecimal digit
        if (pbTable[i] >= '0' && pbTable[i] <= '9')
            // Step 3: Convert the character to its decimal equivalent
            data[i] = pbTable[i] - '0';
        else if (pbTable[i] >= 'A' && pbTable[i] <= 'F')
            // Convert uppercase hexadecimal letters to decimal
            data[i] = pbTable[i] - 'A' + 10;
        else
            // If not a valid character, set it to 0
            data[i] = 0;
    }

    // Step 4: Combine the bytes back into a floating-point value
    value.b[3] = (data[0] << 4) | data[1];
    value.b[2] = (data[2] << 4) | data[3];
    value.b[1] = (data[4] << 4) | data[5];
    value.b[0] = (data[6] << 4) | data[7];

    // Step 5: Return the floating-point value
    return value.f;
}

void LR10KBufferMessage::DataBuildUpBufferMessageFromFloat(float f)
{
    // Set a flag to indicate that this is not a single buffer message
    singleBufferMessage = false;

    // Get a pointer to the current position in the message buffer (8 bits per position)
    byte* data = &message[size_of_message];

    // Create a union to interpret the float as binary data
    UValue value;
    value.f = f;

    // Step 1: Split the 32-bit float into four 8-bit parts and store them in 'data'
    data[0] = value.b[3] >> 4;
    data[1] = value.b[3] & 0x0F;
    data[2] = value.b[2] >> 4;
    data[3] = value.b[2] & 0x0F;
    data[4] = value.b[1] >> 4;
    data[5] = value.b[1] & 0x0F;
    data[6] = value.b[0] >> 4;
    data[7] = value.b[0] & 0x0F;

    // Step 2: Convert the binary values to hexadecimal characters
    for (int i = 0; i < 8; i++)
    {
        if (data[i] <= 9)
            data[i] += '0';  // Convert to ASCII '0'-'9'
        else
        {
            data[i] -= 10;
            data[i] += 'A';  // Convert to ASCII 'A'-'F'
        }
    }

    // Step 3: Update the size of the message buffer
    size_of_message += 8;
}

std::vector<std::pair<byte, bool>> LR10KBufferMessage::GetVectorFromFloat(float f)
{
    byte message2[1000]     = { 0 };
    size_t size_of_message2 = { 0 };

    // Get a pointer to the current position in the message buffer (8 bits per position)
    byte* data = &message2[size_of_message2];

    // Create a union to interpret the float as binary data
    UValue value;
    value.f = f;

    // Step 1: Split the 32-bit float into four 8-bit parts and store them in 'data'
    data[0] = value.b[3] >> 4;
    data[1] = value.b[3] & 0x0F;
    data[2] = value.b[2] >> 4;
    data[3] = value.b[2] & 0x0F;
    data[4] = value.b[1] >> 4;
    data[5] = value.b[1] & 0x0F;
    data[6] = value.b[0] >> 4;
    data[7] = value.b[0] & 0x0F;

    // Step 2: Convert the binary values to hexadecimal characters
    for (int i = 0; i < 8; i++)
    {
        if (data[i] <= 9)
            data[i] += '0';  // Convert to ASCII '0'-'9'
        else
        {
            data[i] -= 10;
            data[i] += 'A';  // Convert to ASCII 'A'-'F'
        }
    }

    size_of_message2 += 8;

    // Make a new vector to return
    std::vector<std::pair<byte, bool>> HEX_sequence = {  };
    for (size_t i = 0; i < size_of_message2; i++)
    {
        HEX_sequence.push_back(std::make_pair(message2[i], false));
    }

    return HEX_sequence;
}

void LR10KBufferMessage::DataBuildUpBufferMessage(std::vector<byte> hexdecimalData, bool clearBuffer)
{
    singleBufferMessage = false;

    if (clearBuffer)
        clearBufferSystem();

    for (int i = 0; i < hexdecimalData.size(); i++)
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
