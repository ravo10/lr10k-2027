/**

    @file      LR10KBufferMessage.h
    @brief     Pre-defines functions for the LR10KBufferMessage.cpp file.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#pragma once

#include <cstdint>          //!< uint8_t
#include <iostream>
#include <string>           //!< std
#include <vector>           //!< std
#include <tuple>            //!< std

/**
    @typedef byte
    @brief   Used for defining the byte type.
**/
typedef uint8_t byte;

/**
    @union UValue
    @brief Used for when converting from float or to float, when dealing with raw LR 10K Tensile Machine data.
**/
union UValue
{
    float f;    //!< A float value which is the input float.
    byte b[4];  //!< An array that will contain four bytes.
};

/**

    @class   LR10KBufferMessage
    @brief   Holds a buffer package/message that will be sent to the LR10K Tensile Machine.
    @details ~

**/
class LR10KBufferMessage
{
public:
    /**
        @brief LR10KBufferMessage object constructor
    **/
    LR10KBufferMessage();
    /**
        @brief LR10KBufferMessage object destructor
    **/
    ~LR10KBufferMessage();

    byte message[1000] = { 0 };     //!< Contains the buffer message sent to LR10K.
    size_t size_of_message = { 0 }; //!< Contains the size of the buffer message.
    byte message_single;            //!< Used for sending a single buffer message.

    /**
        @brief Reset the size count of the buffer message "size_of_message".
    **/
    void ResetSizeOfMessage();

    /**
        @brief  Get the float value from the encrypted data from the LR10K Tensile Machine. // Not in use (?) (not edited for this class)
        @param  pbTable - 
        @retval         - 
    **/
    float FindFloat(byte* pbTable);
    /**
        @brief Convert float values from the GUI to be sent and inserted into the buffer message for the LR10K Tensile Machine.
        @param f - 
    **/
    void DataBuildUpBufferMessageFromFloat(float f);
    /**
        @brief  
        @param  f - 
        @retval   - 
    **/
    std::vector<std::pair<byte, bool>> GetVectorFromFloat(float f);

    /**
        @brief Used if a buffer message needs to be built up and inserted to the buffer message for the LR10K Tensile Machine.
        @param hexdecimalData - 
        @param clearBuffer    - 
    **/
    void DataBuildUpBufferMessage(std::vector<byte> hexdecimalData, bool clearBuffer = false);
    /**
        @brief Used if a buffer message needs to be built up and inserted to the buffer message for the LR10K Tensile Machine.
        @param hexdecimalData - 
    **/
    void DataSetSingleBufferMessage(byte hexdecimalData);

    /**
        @brief  Get current status of buffer message. (single or not)
        @retval  - 
    **/
    bool isBuffSingle();
private:
    //!< [internal] Will be set depending on the last function used (BufferMessage, DataBuildUpBufferMessageFromFloat or SingleBufferMessage)
    bool singleBufferMessage = false;

    /**
        @brief [internal] Clears the message buffer and the size of buffer. (does not clear the single buffer message)
    **/
    void clearBufferSystem();
};
