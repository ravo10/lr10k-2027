/**

    @file      LR10KController.h
    @brief     Pre-defines functions for the LR10KController.cpp file. LR10KBufferMessage as base class for certain functions.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      8.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/
#pragma once

#include <stdio.h>
#include "common.h"
#include "LR10KBufferMessage.h"
/**

    @class   LR10KController
    @brief   Contains the original HEX decimals sequences to control the LR 10K Tensile Machine.
    @details ~

**/
class LR10KController
{
public:
    /** \details
        Releated to the write sequences for the setup
        Setup #1 (happens only one time, but needs to be ran again after the LR 10K is disconnected virtually)
        This will make the machine say it is "Ready" with a yellow light.
    **/
    // Setup #1 (happens only one time, but needs to be ran again after the LR 10K is disconnected virtually)
    //! \note Send this first every time the LR 10K has been disconnected.
    //! \details (bool; false = Write & true = Read (this also enables a wait for response. See the: SendHEXWriteReadSequenceToLR10K function)).
    //!             Another thing noticed, is that the last write byte sent will be sent back from LR 10K until it response to a new byte.
    //!             If the new byte sent does not make the LR 10K respond with a new byte, the old one will be sent out until the LR 10K
    //!             does respond with a new byte. E.g. 0x00 & 0xB6 will be sent out unitl 0xB7 is sent and the new response is 0xBA.
    const std::vector<std::pair<byte, bool>> Write_FirstSetup = {
        /* std::make_pair(0xB6, false),  //!< SEND STATUS */ // This one can not be enabled.. Only for documentation. "AskAndWaitForLR10KForReadyToConnect" function will send this byte.
        //    std::make_pair(0xB0, true),
        //std::make_pair(0xB6, false),  //!< SEND STATUS
        //   std::make_pair(0x0B, true),
        //std::make_pair(0xB6, false),  //!< SEND STATUS
            std::make_pair(0xB6, true),
        std::make_pair(0xC1, false),
            std::make_pair(0x00, true),
        std::make_pair(0xBA, false),
        std::make_pair(0xD6, false),
        std::make_pair(0x0A, false),
        std::make_pair(0xD5, false),
        std::make_pair(0x11, false),
        std::make_pair(0x00, false),
        std::make_pair(0xCC, false),
        std::make_pair(0xCA, false),
        std::make_pair(0x81, false),
        std::make_pair(0xB7, false),
        std::make_pair(0xB0, false),
            std::make_pair(0xBA, true),
            // This is the last things the LR 10K sends out
            std::make_pair(0xB2, true),
            std::make_pair(0x00, true),
            std::make_pair(0xB3, true),
            std::make_pair(0x20, true),
            std::make_pair(0xB1, true),
            std::make_pair(0x03, true),
            std::make_pair(0xB7, true),
            std::make_pair(0x33, true),
            std::make_pair(0x39, true),
            std::make_pair(0x38, true),
            std::make_pair(0x36, true),
            std::make_pair(0x44, true),
            std::make_pair(0x31, true),
            std::make_pair(0x34, true),
            std::make_pair(0x32, true) // Could only check this, but for documentation purposes and loading overview..
    };

    // Setup #2 (happens every time a user wants to change any settings for a test e.g. Test Speed 1, Jog Speed etc.)
    //! \details Needs to be sent before a new second setup is going to be sent.
    const std::vector<std::pair<byte, bool>> Write_EditSecondSetup   = {
        std::make_pair(0xB0, false),
            //std::make_pair(0xB0, true),
            //std::make_pair(0x0B, true)
    };
    //! \note This is WITHOUT Extensometer
    //! \details (default) This defines the setup that will be used under testing, and it will change every time a new settings is set.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_Begin = {
        std::make_pair(0xB6, false),  //!< SEND STATUS
            //std::make_pair(0xB6, true),
            //std::make_pair(0x00, true),
        std::make_pair(0xD1, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x07, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x34, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x33, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x37, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x41, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x30, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x30, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x30, false),  //!< SETUP TO RECEIVE  
        std::make_pair(0x30, false)   //!< SETUP TO RECEIVE 
    };
    //! \details This is the speed of the crosshead during an active/started test.
    //! \details The last 8 bytes must be replaced with a dynamic float/double value using the encrypter. (default = 100 mm/min)
    //! \note Dynamic values will be inserted/changed in the setup function. This needs to be added after this.
    std::vector<std::pair<byte, bool>> Write_SecondSetup_TestSpeed1  = {
        std::make_pair(0xD3, false),  //!< TEST SPEED 1 | Confirmed  
        std::make_pair(0x07, false),  //!< TEST SPEED 1 | Confirmed  
        std::make_pair(0x00, false),  //!< TEST SPEED 1 | Confirmed  
        std::make_pair(0xD1, false),  //!< TEST SPEED 1 | Confirmed  
        std::make_pair(0x02, false)   //!< TEST SPEED 1 | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)    
    };
    // "Dynamic HEX Numbers sequence here"
    //! \brief This is the speed of the crosshead when translating/jogging up/down before a test is started.
    //! \details The last 8 bytes must be replaced with a dynamic float/double value using the encrypter.
    //! \note (default = 100 mm/min)
    //! \note Dynamic values will be inserted/changed in the setup function. This needs to be added after this.
    std::vector<std::pair<byte, bool>> Write_SecondSetup_JogSpeed    = {
        std::make_pair(0xD3, false),  //!< JOG SPEED | Confirmed  
        std::make_pair(0x02, false),  //!< JOG SPEED | Confirmed  
        std::make_pair(0x02, false),  //!< JOG SPEED | Confirmed  
        std::make_pair(0xD1, false),  //!< JOG SPEED | Confirmed  
        std::make_pair(0x06, false)   //!< JOG SPEED | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
    };
    // "Dynamic HEX Numbers sequence here"
    //! \brief Tells wether it will be a tensile or compression test.
    //! \details The last byte: Tensile = 0x00 and Compression = 0x01
    //! \note (default = Tensile)
    //! \note Dynamic values will be inserted/changed in the setup function. This needs to be added after this.
    std::vector<std::pair<byte, bool>> Write_SecondSetup_TestType    = {
        std::make_pair(0xD3, false),  //!< COMPRESSION/TENSILE | Confirmed  
        std::make_pair(0x06, false),  //!< COMPRESSION/TENSILE | Confirmed  
        std::make_pair(0x02, false),  //!< COMPRESSION/TENSILE | Confirmed  
        std::make_pair(0xD5, false),  //!< COMPRESSION/TENSILE | Confirmed  
        std::make_pair(0x01, false)   //!< COMPRESSION/TENSILE | Confirmed (The next 1 byte comes from a integer value)   
    };
    // "Dynamic HEX Numbers sequence here"
    //! \note This is WITH and WITHOUT Extensometer.
    //! \details (default) Sets the mid-data for the second setup.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_Mid         = {
        std::make_pair(0xD5, false),
        std::make_pair(0x02, false),
        std::make_pair(0x01, false),
        std::make_pair(0xD5, false),
        std::make_pair(0x03, false),
        std::make_pair(0x01, false),
        std::make_pair(0xD5, false),
        std::make_pair(0x04, false),
        std::make_pair(0x00, false),
        std::make_pair(0xD5, false),
        std::make_pair(0x07, false),
        std::make_pair(0x00, false),
        std::make_pair(0xD5, false),
        std::make_pair(0x0E, false),
        std::make_pair(0x00, false),
        std::make_pair(0xB0, false)
    };
    //! \note This is WITHOUT Extensometer.
    //! \brief Part 1/3.
    //! \details (default) Sets the end-data for the second setup. This sequence also seems to set the zero point automatically inside LR 10K.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_End1         = {
            //std::make_pair(0xB0, true), // Could be only needed for when axis limits are set...
            //std::make_pair(0x0B, true),
        std::make_pair(0xD1, false),  //!< EXTENSION  | Confirmed
        std::make_pair(0x04, false),  //!< EXTENSION  | Confirmed  (The next 8 bytes comes from a changing double (15 decimal digits) value)  
    };
    // "Dynamic HEX Numbers sequence here"
    //! \note This is WITHOUT Extensometer.
    //! \brief Part 2/3.
    //! \details (default) Sets the end-data for the second setup. This sequence also seems to set the zero point automatically inside LR 10K.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_End2 = {
        std::make_pair(0xD3, false),  //!< EXTENSION  | Confirmed
        std::make_pair(0x04, false),  //!< EXTENSION  | Confirmed  
        std::make_pair(0x00, false),  //!< EXTENSION  | Confirmed  
        std::make_pair(0xD1, false),  //!< EXTENSION  | Confirmed  
        std::make_pair(0x05, false),  //!< EXTENSION  | Confirmed  (The next 8 bytes comes from a changing double (15 decimal digits) value)  
    };
    // "Dynamic HEX Numbers sequence here"
    //! \note This is WITHOUT Extensometer.
    //! \brief Part 3/3.
    //! \details (default) Sets the end-data for the second setup. This sequence also seems to set the zero point automatically inside LR 10K.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_End3 = {
        std::make_pair(0xD3, false),  //!< EXTENSION  | Confirmed
        std::make_pair(0x05, false),  //!< EXTENSION  | Confirmed  
        std::make_pair(0x00, false),  //!< EXTENSION  | Confirmed  
        std::make_pair(0xD1, false),  //!< EXTENSION  | Confirmed  
        std::make_pair(0x01, false),  //!< EXTENSION  | Confirmed  (The next 8 bytes comes from a changing double (15 decimal digits) value)  
    };
    // "Dynamic HEX Numbers sequence here"
    //! \note This is WITH and WITHOUT Extensometer.
    //! \brief Part 1/3.
    //! \details (default) Sets the end-data for the second setup. This sequence also seems to set the zero point automatically inside LR 10K.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_End4_1 = {
        std::make_pair(0xD3, false),
        std::make_pair(0x01, false),
        std::make_pair(0x00, false)
    };
    //! \note This is WITHOUT Extensometer.
    //! \brief Part 2/3.
    //! \details (default) Sets the end-data for the second setup. This sequence also seems to set the zero point automatically inside LR 10K. This part is uniqe for WITHOUT extensometer.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_End4_2 = {
        std::make_pair(0xD5, false),
        std::make_pair(0x09, false),
        std::make_pair(0x00, false)
    };
    //! \note This is WITH and WITHOUT Extensometer.
    //! \brief Part 3/3.
    //! \details (default) Sets the end-data for the second setup. This sequence also seems to set the zero point automatically inside LR 10K.
    const std::vector<std::pair<byte, bool>> Write_SecondSetup_End4_3 = {
        std::make_pair(0xD1, false),
        std::make_pair(0x00, false),
        std::make_pair(0x34, false),
        std::make_pair(0x36, false),
        std::make_pair(0x31, false),
        std::make_pair(0x43, false),
        std::make_pair(0x34, false),
        std::make_pair(0x30, false),
        std::make_pair(0x30, false),
        std::make_pair(0x30, false),
        std::make_pair(0xD3, false),
        std::make_pair(0x00, false),
        std::make_pair(0x04, false),
        std::make_pair(0xCA, false),
        std::make_pair(0xB1, false),
            /*std::make_pair(0xB2, true),
            std::make_pair(0x00, true),
            std::make_pair(0xB3, true),
            std::make_pair(0x20, true),
            std::make_pair(0xB1, true),
            std::make_pair(0x03, true),*/
        std::make_pair(0xB2, false),
            /*std::make_pair(0xB1, true),
            std::make_pair(0x03, true),
            std::make_pair(0xB2, true),
            std::make_pair(0x00, true),*/
        std::make_pair(0xB3, false),
            /*std::make_pair(0xB3, true),
            std::make_pair(0x20, true),*/
        std::make_pair(0x82, false),
            /*std::make_pair(0xB2, true),
            std::make_pair(0x00, true),
            std::make_pair(0xB3, true),
            std::make_pair(0x20, true),*/
        std::make_pair(0xC0, false),  //!< SEND LIVE DATA TO PC
            /*std::make_pair(0xB1, true),
            std::make_pair(0x03, true),*/
        std::make_pair(0xC0, false)  //!< SEND LIVE DATA TO PC
    };
    //! \warning This is WITH and WITHOUT Extensometer.
    //! \details Sets the mid-data for the second setup, before the extension data will be added.
    const std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_Mid = Write_SecondSetup_Mid;
    //! \warning This is WITH Extensometer.
    //! \brief Part 1/4. Sets up the extensiometer data, where the range can be inputed after.
    //! \details The last 8 bytes must be replaced with a dynamic float/double value using the encrypter.
    //! \note (default = 1000 mm)
    //! \note Dynamic values will be inserted/changed in the setup function. This needs to be added after this.
    std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_ExtSetup1      = {
        std::make_pair(0xD5, false),  //!< EXTENSOMETER RANGE | Confirmed  
        std::make_pair(0x09, false),  //!< EXTENSOMETER RANGE | Confirmed  
        std::make_pair(0x00, false),  //!< EXTENSOMETER RANGE | Confirmed  
        std::make_pair(0xD1, false),  //!< EXTENSOMETER RANGE | Confirmed  
        std::make_pair(0x03, false),  //!< EXTENSOMETER RANGE | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)  
    };
    // "Dynamic HEX Numbers sequence here"
    //! \warning This is WITH Extensometer.
    //! \brief Part 2/4.
    //! \details Sets up the distance/extensiometer data, where the distance can be inputed after.
    //! \note There could be more settings to add here later, but non was detected when settings were changed - This could be false tho.
    const std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_ExtSetup2 = {
        // 1 (uniqe)
        std::make_pair(0xD3, false),  //!< EXTENSION (uniqe sequence for Ext.meter) | Confirmed
        std::make_pair(0x03, false),  //!< EXTENSION (uniqe sequence for Ext.meter) | Confirmed
        std::make_pair(0x00, false),  //!< EXTENSION (uniqe sequence for Ext.meter) | Confirmed
        std::make_pair(0xD1, false),  //!< EXTENSION (uniqe sequence for Ext.meter) | Confirmed
        std::make_pair(0x04, false),  //!< EXTENSION (uniqe sequence for Ext.meter) | Confirmed (The next 8 bytes comes from a changing double (15 decimal digits) value)
    };
    // "Dynamic HEX Numbers sequence here"
    //! \warning This is WITH Extensometer.
    //! \brief Part 3/4.
    //! \details Sets the end-data for the second setup.
    const std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_ExtSetup3 = Write_SecondSetup_End2;
    // "Dynamic HEX Numbers sequence here"
    //! \warning This is WITH Extensometer.
    //! \brief Part 4/4.
    //! \details Sets the end-data for the second setup.
    const std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_ExtSetup4 = Write_SecondSetup_End3;
    // "Dynamic HEX Numbers sequence here"
    //! \warning This is WITH Extensometer.
    //! \brief Part 1/2.
    //! \details Sets the end-data for the second setup.
    const std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_End1 = Write_SecondSetup_End4_1;
    //! \warning This is WITH Extensometer.
    //! \brief Part 2/2.
    //! \details Sets the end-data for the second setup.
    const std::vector<std::pair<byte, bool>> Write_SecondSetupWExt_End2 = Write_SecondSetup_End4_3;

    // Releated to the write sequences for the control panel
    //! \details Cancel a pre-loaded test before it is started.
    const std::vector<std::pair<byte, bool>> Write_CancelPreLoadedTest = { std::make_pair(0x81, false), std::make_pair(0xB6, false)/*, 0xC1*/ };               
    const std::vector<std::pair<byte, bool>> Write_Stop                = { std::make_pair(0x81, false), std::make_pair(0xB6, false) };                                                                                              //!< \details Stop the translation of the crosshead.
    const std::vector<std::pair<byte, bool>> Write_Up                  = { std::make_pair(0x84, false) };                                                                                                                           //!< \details Translate the crosshead up.
    const std::vector<std::pair<byte, bool>> Write_Down                = { std::make_pair(0x85, false) };                                                                                                                           //!< \details Translate the crosshead down.
    const std::vector<std::pair<byte, bool>> Write_FastUpPress         = { std::make_pair(0x81, false), std::make_pair(0xB6, false), std::make_pair(0x86, false) };                                                                 //!< \details Translate the crosshead up fast.
    const std::vector<std::pair<byte, bool>> Write_FastUpRelease       = { std::make_pair(0x81, false), std::make_pair(0xB6, false), std::make_pair(0x81, false), std::make_pair(0xB6, false) };                                    //!< \details Stop all translations after the crosshead moved up fast.
    const std::vector<std::pair<byte, bool>> Write_FastDownPress       = { std::make_pair(0x81, false), std::make_pair(0xB6, false), std::make_pair(0x87, false) };                                                                 //!< \details Translate the crosshead down fast.
    const std::vector<std::pair<byte, bool>> Write_FastDownRelease     = Write_FastUpRelease;                                                                                                                                       //!< \details Stop all translations after the crosshead moved down fast.
    const std::vector<std::pair<byte, bool>> Write_ReturnZero          = { std::make_pair(0x83, false) };                                                                                                                           //!< \details Return the crosshead to its zero point.
    const std::vector<std::pair<byte, bool>> Write_SetZero             = { std::make_pair(0xB6, false), std::make_pair(0x82, false), std::make_pair(0xC0, false)};                                                                  //!< \details Set a new zero point for the crosshead.
    const std::vector<std::pair<byte, bool>> Write_Go                  = { std::make_pair(0x81, false), std::make_pair(0xB6, false), std::make_pair(0xC1, false), std::make_pair(0xC0, false), std::make_pair(0x9F , false)};       //!< \details Start a new test based on the current setup.
    const std::vector<std::pair<byte, bool>> Write_PauseTest           = { std::make_pair(0x81, false), std::make_pair(0xB6, false), std::make_pair(0xC1, false)};                                                                  //!< \details Pause an active/started test.
    const std::vector<std::pair<byte, bool>> Write_ContinueTest        = { std::make_pair(0xC0, false), std::make_pair(0x9F, false)};                                                                                               //!< \details Continue a pause test that is active/started.
    const std::vector<std::pair<byte, bool>> Write_EndTest             = Write_CancelPreLoadedTest;                                                                                                                                 //!< \details End an active/started test (success).

    // Releated to the write sequences when the user wants to disconnect the LR 10K
    //! \details Needs to be sent before the program closes, or the LR 10K should be disconnected from software.
    const std::vector<std::pair<byte, bool>> Write_DisconnectLR10K     = {
        std::make_pair(0x81, false),
        std::make_pair(0xCD, false),
        std::make_pair(0x80, false)
    };

public:
    // w = Write, r = Read

    /**
        @brief  @brief  Basic setup to send a write/read command - Similar for many scenarios.
        @details Will wait to write and send more HEX sequence if there is a read HEX we want the LR 10K to send, before continuing.
        @param  HEXSequence                           - 
        @param  sendPercentageFinished                - 
        @param  allowSendingDataEvenIfMainLoopIsEnded - 
        @retval                                       - 
    **/
    bool SendHEXWriteReadSequenceToLR10K(std::vector<std::pair<byte, bool>> HEXSequence, bool sendPercentageFinished = false, bool allowSendingDataEvenIfMainLoopIsEnded = false);

    /**
        @brief Asks if LR 10K if it is ready to connect, and waits for a 0xB6 response which confirms it.
    **/
    bool AskAndWaitForLR10KForReadyToConnect();

    /**
        @brief  Sends setup 1  (the one that makes the LR 10K "Ready")
        @retval  - 
    **/
    bool w_Setup1();
    /**
        @brief  Sends setup 2 (the one that adjusts the settings for a test)
        @param  testSpeed1            - 
        @param  jogSpeed              - 
        @param  tensileTest           - 
        @param  useExtensometer      - 
        @param  extensometerDistance - 
        @retval                       - 
    **/
    bool w_Setup2(
        float testSpeed1                = 100.0f
        , float jogSpeed                = 100.0f
        , bool  tensileTest             = true
        , bool  useExtensometer        = false
        , float extensometerDistance   = 1000.0f
    );

    /**
        @brief Save settings sent to the LR 10K Tensile Machine.
        @details It simulates a very quick "Go" command with a "End test" 10 ms after. This makes the LR 10K apply the settings.
    **/
    void w_SaveSettingsSent();
    /**
        @brief  Cancel a pre-loaded test.
        @retval  - 
    **/
    bool w_CancelPreLoadedTest();
    /**
        @brief  Stop translation of crosshead.
        @retval  - 
    **/
    bool w_Stop();
    /**
        @brief  Translate the crosshead upwards.
        @retval  - 
    **/
    bool w_Up();
    /**
        @brief  Translate the crosshead downwards.
        @retval  - 
    **/
    bool w_Down();
    /**
        @brief  Start moving the crosshead fast upwards.
        @retval  - 
    **/
    bool w_FastUpPress();
    /**
        @brief  Stop the fast movement of the crosshead.
        @retval  - 
    **/
    bool w_FastUpRelease();
    /**
        @brief  Start moving the crosshead fast downwards.
        @retval  - 
    **/
    bool w_FastDownPress();
    /**
        @brief  Stop the fast movement of the crosshead.
        @retval  - 
    **/
    bool w_FastDownRelease();
    /**
        @brief  Return the crosshead to it last set zero point.
        @retval  - 
    **/
    bool w_ReturnZero();
    /**
        @brief  Set the zero point for the crosshead (load and distance)
        @retval  - 
    **/
    bool w_SetZero();
    /**
        @brief  Start a new test based on the settings set in setup 2.
        @retval  - 
    **/
    bool w_Go();
    /**
        @brief  Pause a test that has been started.
        @retval  - 
    **/
    bool w_PauseTest();
    /**
        @brief  Continue a test that has been paused, after it has been started.
        @retval  - 
    **/
    bool w_ContinueTest();
    /**
        @brief  End a started test.
        @retval  - 
    **/
    bool w_EndTest();
    /**
        @brief  Abort a started test.
        @retval  - 
    **/
    bool w_AbortTest();

    /**
        @brief  Disconnect the LR 10K Machine programatically. (same as setup #2)
        @retval  - 
    **/
    bool w_Disconnect();
};
