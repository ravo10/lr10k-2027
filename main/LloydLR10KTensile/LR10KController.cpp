/**

    @file      LR10KController.cpp
    @brief     Contains HEX data packages to control the LR 10K Tensile Machine's beahvior (Write).
                E.g. translate the crosshead, start a new test or disconnect the LR 10K.
                These data packages are "sniffed" from the COM port using PortMon 3.0.2.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      8.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "LR10KController.h"


bool LR10KController::SendHEXWriteReadSequenceToLR10K(std::vector<std::pair<byte, bool>> HEXSequence, bool sendPercentageFinished, bool allowSendingDataEvenIfMainLoopIsEnded)
{
    if (!allowSendingDataEvenIfMainLoopIsEnded && mainGUIGlobals::done) return false;

    // -------------------------------------------------------------------------
    // Get percentage completed based on that we know every return value LR 10K should send back
    // -------------------------------------------------------------------------

    std::string percentageComplete = "0 %%";
    LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>(percentageComplete.c_str()));
    double totalAmountOfReadBytes = 0.0;

    // Make nice percentage text
    auto getPercentageFinsished = [](double amountOfReadBytes, double totalAmountOfReadBytes) -> std::string {
        // Make nice percentage text
        char roundedValueChar[30];
        sprintf_s(roundedValueChar, "%.0f", (amountOfReadBytes / totalAmountOfReadBytes) * 100);

        std::string percentageComplete = roundedValueChar;
        percentageComplete.append(" %%");

        return percentageComplete;
    };

    for (size_t i = 0; i < HEXSequence.size(); i++)
    {
        // Get the HEX sequence
        std::pair<byte, bool> HEX = HEXSequence[i];
        bool HEX_read = std::get<bool>(HEX);

        if (HEX_read)
            totalAmountOfReadBytes++;
    }

    // -------------------------------------------------------------------------
    // ------------------------- Start processing bytes ------------------------
    // -------------------------------------------------------------------------

    LR10KBufferMessage bufferMessage;

    bool clearBuffer = false;
    double amountOfReadBytes = 0.0;

    for (size_t i = 0; i < HEXSequence.size(); i++)
    {
        if (LR10KAppGlobals::connectedComPortNumber == -1)
        {
            
            if (!mainGUIGlobals::useDemoMode)
            {
                LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>("NA"));
                LR10KControlSystem::TryDisconnectFromLR10K();
            }
            else
            {
                // Reset GUI variabels
                LR10KAppGlobals::configurationDone = false;

                LR10KAppGlobals::sendingLiveDataFromLR10K = false;
                LR10KAppGlobals::currentLiveStatusMessage = "N/A";
                LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("N/A"));
                DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
                LR10KAppGlobals::currentWarnErrorStatusMessage = "N/A";

                TabControl::LR10KRawLoad = 0.0f;
                TabControl::LR10KRawExtension = 0.0f;
                TabControl::LR10KRawExtensometerRange = 0.0f;

                TabControl::LR10KOffsetLoad = 0.0f;
                TabControl::LR10KOffsetExtension = 0.0f;
                TabControl::LR10KOffsetExtensometerRange = 0.0f;

                TabControl::LR10KLoadNice = "0.0";
                TabControl::LR10KExtensionNice = "0.0";
                TabControl::LR10KExtensometerRangeNice = "0.0";

                TabControl::LR10KTravelDistanceControl = 0.0f;
                TabControl::LR10KTravelDistanceActionAfter = 0;
            }

            return false;
        }

        // Get the HEX sequence
        std::pair<byte, bool> HEX = HEXSequence[i];

        // Build up message until there is something we need to wait on...
        byte HEX_byte = std::get<byte>(HEX);
        bool HEX_read = std::get<bool>(HEX);

        if (HEX_read)
        {
            amountOfReadBytes++;
            percentageComplete = getPercentageFinsished(amountOfReadBytes, totalAmountOfReadBytes);
            if (LR10KCOMSystem::CheckIfCOMIsConnected())
            {
                LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>(percentageComplete.c_str()));
                Sleep(6); // !!Important!!

                // Wait for the correct response from LR 10K...
                LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
                LR10KCOMSystem::WaitAndReadFromCOM(HEX_byte);
            }

            // Clear buffer
            clearBuffer = true;
        }
        else
        {
            // Build up message
            bufferMessage.DataBuildUpBufferMessage({ HEX_byte }, clearBuffer);
            clearBuffer = false;
        }
    }

    // If there is something left to be written
    if (LR10KCOMSystem::CheckIfCOMIsConnected() && bufferMessage.size_of_message > 0)
        return LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);

    return true;
}

bool LR10KController::AskAndWaitForLR10KForReadyToConnect()
{
    LR10KBufferMessage bufferMessage;
    bufferMessage.DataSetSingleBufferMessage(0xB6);

    bool LR10KReadyToConnect = false;

    // Ask LR 10K if it is ready to connect
    int counter = 0;
    while (!LR10KReadyToConnect)
    {
        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);

        byte uInfo = { 0 };
        BOOL successPeak = LR10KCOMSystem::PeekDataFromCom(LR10KDataDeEncryptionSystem::m_hSerial, uInfo);

        std::cout << "Trying to get a response from LR 10K (" << (byte)0xB6 << "): " << uInfo << std::endl;

        // Check if the LR 10K says it is ready
        LR10KReadyToConnect = uInfo == (byte)0xB6 ? true : false;

        if (counter >= 10 * 3)
            break;

        // 10 Hz
        Sleep(30); counter++;
    }

    return LR10KReadyToConnect;
}

bool LR10KController::w_Setup1()
{
    // Wait until LR 10K is ready to connect
    bool continueSetup = AskAndWaitForLR10KForReadyToConnect();
    if (!continueSetup)
    {
        mainGUIGlobals::showPopUp_ConnectionTimeout = true;

        return false;
    }

    // Create setup sequence
    std::vector<std::pair<byte, bool>> FirstSetupComplete = {  };
    FirstSetupComplete.insert(FirstSetupComplete.end(), Write_FirstSetup.begin(), Write_FirstSetup.end());

    return SendHEXWriteReadSequenceToLR10K(FirstSetupComplete);
}

bool LR10KController::w_Setup2(float testSpeed1, float jogSpeed, bool tensileTest, bool useExtensometer, float extensometerDistance)
{
    LR10KBufferMessage bufferMessage;

    // Tell the LR 10K Machine that there is a new setup incomming
    bufferMessage.DataSetSingleBufferMessage(std::get<byte>(Write_EditSecondSetup[0]));
    if (LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial))
    {
        // Create setup sequence
        std::vector<std::pair<byte, bool>> SecondSetupComplete = {  };

        // -------------------------------------------------------------------------
        // ----------------------------- Dynamic values ----------------------------
        // -------------------------------------------------------------------------

        // Get all of the dynamic values from float++
        // Test speed 1, Jog Speed and Test type
        std::vector<std::pair<byte, bool>> HEX_sequence_testSpeed1 = bufferMessage.GetVectorFromFloat(testSpeed1);
        std::vector<std::pair<byte, bool>> HEX_sequence_jogSpeed = bufferMessage.GetVectorFromFloat(jogSpeed);
        std::vector<std::pair<byte, bool>> HEX_sequence_testTyp = { std::make_pair((byte)(tensileTest ? 0x00 : 0x01), false) };
        // Distance/Extension
        float CustomExtensionAlgorithmValue = LR10KDataDeEncryptionSystem::convertAlgorithmForExtension(LR10KAppGlobals::maximumExtensionAllowedInsideLR10K);
        std::vector<std::pair<byte, bool>> HEX_sequence_Extension = bufferMessage.GetVectorFromFloat(CustomExtensionAlgorithmValue);
        // Extensometer (optional)
        std::vector<std::pair<byte, bool>> HEX_sequence_extensometerDistance = bufferMessage.GetVectorFromFloat(extensometerDistance);

        // -------------------------------------------------------------------------

        // Build up the setup message
        SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_Begin.begin(), Write_SecondSetup_Begin.end());
        // Dynamic values 1
        SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_TestSpeed1.begin(), Write_SecondSetup_TestSpeed1.end());
        SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_testSpeed1.begin(), HEX_sequence_testSpeed1.end());
        // Dynamic values 2
        SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_JogSpeed.begin(), Write_SecondSetup_JogSpeed.end());
        SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_jogSpeed.begin(), HEX_sequence_jogSpeed.end());
        // Dynamic values 3
        SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_TestType.begin(), Write_SecondSetup_TestType.end());
        SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_testTyp.begin(), HEX_sequence_testTyp.end());

        // -------------------------------------------------------------------------
        // --------------- Create rest of setup sequence based on if ---------------
        // ------------------- extensometer will be added or not -------------------
        // -------------------------------------------------------------------------

        if (!useExtensometer)
        {
            // ---------------------------------------------------------------------
            // ----------------------- WITHOUT Extensometer ------------------------
            // ---------------------------------------------------------------------
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_Mid.begin(), Write_SecondSetup_Mid.end());
            // Dynamic values 4
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_End1.begin(), Write_SecondSetup_End1.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_Extension.begin(), HEX_sequence_Extension.end()); // Three times...
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_End2.begin(), Write_SecondSetup_End2.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_Extension.begin(), HEX_sequence_Extension.end()); // Three times...
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_End3.begin(), Write_SecondSetup_End3.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_Extension.begin(), HEX_sequence_Extension.end()); // Three times...
            // End setup
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_End4_1.begin(), Write_SecondSetup_End4_1.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_End4_2.begin(), Write_SecondSetup_End4_2.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetup_End4_3.begin(), Write_SecondSetup_End4_3.end());
        }
        else
        {
            // ---------------------------------------------------------------------
            // -------------------- WITH Extensometer (optional) -------------------
            // ---------------------------------------------------------------------
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_Mid.begin(), Write_SecondSetupWExt_Mid.end());
            // Dynamic values 4
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_ExtSetup1.begin(), Write_SecondSetupWExt_ExtSetup1.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_extensometerDistance.begin(), HEX_sequence_extensometerDistance.end());
            // Dynamic values 5
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_ExtSetup2.begin(), Write_SecondSetupWExt_ExtSetup2.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_Extension.begin(), HEX_sequence_Extension.end()); // Three times...
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_ExtSetup3.begin(), Write_SecondSetupWExt_ExtSetup3.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_Extension.begin(), HEX_sequence_Extension.end()); // Three times...
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_ExtSetup4.begin(), Write_SecondSetupWExt_ExtSetup4.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), HEX_sequence_Extension.begin(), HEX_sequence_Extension.end()); // Three times...
            // End setup
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_End1.begin(), Write_SecondSetupWExt_End1.end());
            SecondSetupComplete.insert(SecondSetupComplete.end(), Write_SecondSetupWExt_End2.begin(), Write_SecondSetupWExt_End2.end());
        }

        // Send the setup message
        if (SendHEXWriteReadSequenceToLR10K(SecondSetupComplete, true))
        {
            w_SaveSettingsSent();

            return true;
        }
    }

    return false;
}
void LR10KController::w_SaveSettingsSent()
{
    // One way to set the settings is to start a test and end it really quick
    w_Go(); Sleep(5); w_EndTest();
}

bool LR10KController::w_CancelPreLoadedTest()
{
    return SendHEXWriteReadSequenceToLR10K(Write_CancelPreLoadedTest);
}
bool LR10KController::w_Stop()
{
    return SendHEXWriteReadSequenceToLR10K(Write_Stop);
}
bool LR10KController::w_Up()
{
    LR10KAppGlobals::shutLR10KDownIfLoadToHigh = true;
    return SendHEXWriteReadSequenceToLR10K(Write_Up);
}
bool LR10KController::w_Down()
{
    LR10KAppGlobals::shutLR10KDownIfLoadToHigh = true;
    return SendHEXWriteReadSequenceToLR10K(Write_Down);
}
bool LR10KController::w_FastUpPress()
{
    LR10KAppGlobals::shutLR10KDownIfLoadToHigh = true;
    return SendHEXWriteReadSequenceToLR10K(Write_FastUpPress);
}
bool LR10KController::w_FastUpRelease()
{
    return SendHEXWriteReadSequenceToLR10K(Write_FastUpRelease);
}
bool LR10KController::w_FastDownPress()
{
    LR10KAppGlobals::shutLR10KDownIfLoadToHigh = true;
    return SendHEXWriteReadSequenceToLR10K(Write_FastDownPress);
}
bool LR10KController::w_FastDownRelease()
{
    return SendHEXWriteReadSequenceToLR10K(Write_FastDownRelease);
}
bool LR10KController::w_ReturnZero()
{
    return SendHEXWriteReadSequenceToLR10K(Write_ReturnZero);
}
bool LR10KController::w_SetZero()
{
    return SendHEXWriteReadSequenceToLR10K(Write_SetZero);
}
bool LR10KController::w_Go()
{
    LR10KAppGlobals::shutLR10KDownIfLoadToHigh = false;
    return SendHEXWriteReadSequenceToLR10K(Write_Go);
}
bool LR10KController::w_PauseTest()
{
    return SendHEXWriteReadSequenceToLR10K(Write_PauseTest);
}
bool LR10KController::w_ContinueTest()
{
    return SendHEXWriteReadSequenceToLR10K(Write_ContinueTest);
}
bool LR10KController::w_EndTest()
{
    return SendHEXWriteReadSequenceToLR10K(Write_EndTest);
}
bool LR10KController::w_AbortTest()
{
    return w_Setup2();
}

bool LR10KController::w_Disconnect()
{
    return SendHEXWriteReadSequenceToLR10K(Write_DisconnectLR10K, false, true);
}

/*
TODO::
Need to test and adjust software against new functionallity:
*
* **Add possibility to enable extensometer and it's range
*   DONE It, but need an extensometer to learn how it works to enable it fully in code...
*   -->>>I need to know how it works, so it is possible to read the data, and output it in the correct way in GUI etc. etc.
*
* Kan legge til bruddforlengelse (side 25)
* Kan legge til bruddkontraksjonen; bare for runde prøvestaver (side 25)
* 
* Ny verdi etter lagring er ikkje nøyaktig...?
* Må trekke frå nytt null punkt frå extension på export av Excel-ark (defaultValuesGUI::offsetZeroPointExtensionAxis) | 
* Konvertere filter verdi frå mm til inches når ein bytter SI-enhet | 
*
*/
