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

#include <algorithm>
#include <array>


bool LR10KController::SendHEXWriteReadSequenceToLR10K(std::vector<std::pair<byte, bool>> HEXSequence, bool sendPercentageFinished, bool allowSendingDataEvenIfMainLoopIsEnded)
{
    if (!allowSendingDataEvenIfMainLoopIsEnded && mainGUIGlobals::done) return false;

    // -------------------------------------------------------------------------
    // Get percentage completed based on that we know every return value LR 10K should send back
    // -------------------------------------------------------------------------

    // The status text is read by the GUI thread through a raw char pointer, so it must point at memory
    // that stays valid. (It used to point into a local std::string, which was freed when this function
    // returned or the string was reassigned: the GUI then read freed memory.)
    static const std::array<std::string, 101> percentageTexts = [] {
        std::array<std::string, 101> texts;
        for (size_t p = 0; p < texts.size(); p++)
            texts[p] = std::to_string(p) + " %%";
        return texts;
    }();
    auto getPercentageFinsished = [](double amountOfReadBytes, double totalAmountOfReadBytes) -> char* {
        double percent = totalAmountOfReadBytes > 0.0 ? (amountOfReadBytes / totalAmountOfReadBytes) * 100.0 : 100.0;
        size_t index = (size_t)(std::clamp)(percent + 0.5, 0.0, 100.0);
        return const_cast<char*>(percentageTexts[index].c_str());
    };

    LR10KAppGlobals::currentStatusMessage.store(getPercentageFinsished(0.0, 1.0));

    // -------------------------------------------------------------------------
    // ------------------------- Start processing bytes ------------------------
    // -------------------------------------------------------------------------
    // The send/expect logic itself is LR10KMachine::RunSequence (unchanged behaviour, tested against a
    // recorded session in tests/connect_replay_test.cpp).
    const auto result = LR10KDataDeEncryptionSystem::machine.RunSequence(
        std::vector<std::pair<std::uint8_t, bool>>(HEXSequence.begin(), HEXSequence.end()),
        // Send: through the COM port (logged; a failed write is only reported, like before)
        [](const std::uint8_t* data, std::size_t size) { return LR10KCOMSystem::SendBytesToCom(data, size); },
        // Wait at most this long for each answer
        std::chrono::milliseconds(15000),
        // Keep going while the port is open and the COM reader runs
        []() { return LR10KAppGlobals::connectedComPortNumber != -1 && !LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag.load(); },
        // Progress in the status text ("45 %")
        [&](int reply, int replies) { LR10KAppGlobals::currentStatusMessage.store(getPercentageFinsished(reply, replies)); });

    switch (result.outcome)
    {
    case LR10KMachine::SequenceOutcome::Completed:
        return true;

    case LR10KMachine::SequenceOutcome::StoppedBeforeStep:
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
        }
        return false;

    case LR10KMachine::SequenceOutcome::NoReply:
        DockingApps::console.AddLog("[error] LR 10K did not answer (waited 15000 ms for 0x%02X, reply %d of %d). Check the cable and that \"Remote control\" is on.",
            result.waitingFor, result.repliesReceived + 1, result.repliesExpected);
        [[fallthrough]];
    case LR10KMachine::SequenceOutcome::StoppedWhileWaiting:
        LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>("No response"));
        DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
        return false;

    case LR10KMachine::SequenceOutcome::FinalSendFailed:
    default:
        return false;
    }
}

bool LR10KController::AskAndWaitForLR10KForReadyToConnect()
{
    // The handshake itself is LR10KMachine::AskReadyToConnect (tested against a recorded session):
    // send 0xB6 up to 31 times until the LR 10K answers 0xB6.
    const auto result = LR10KDataDeEncryptionSystem::machine.AskReadyToConnect(
        [](const std::uint8_t* data, std::size_t size) { return LR10KCOMSystem::SendBytesToCom(data, size); });

    // Shown in the app's console (Release builds have no printf window), to tell the cases apart:
    // 0 bytes = nothing arrives (wrong port, cable, machine off, "Remote control" off);
    // bytes but no 0xB6 = the machine talks, but does not answer the connect request.
    if (result.ready)
        DockingApps::console.AddLog("LR 10K answered the connect request (after %d tries).", result.requests);
    else if (result.bytesReceived == 0)
        DockingApps::console.AddLog("[error] No data at all from COM%d during %d connect requests. Check the port number, the cable, that the LR 10K is on and that \"Remote control\" is pressed.", LR10KAppGlobals::connectedComPortNumber, result.requests);
    else
        DockingApps::console.AddLog("[error] Received %d bytes from COM%d but no answer (0xB6) to the connect request; last byte was 0x%02X. Try restarting the LR 10K.", result.bytesReceived, LR10KAppGlobals::connectedComPortNumber, result.lastByte);

    return result.ready;
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

    // The sequence starts by waiting for 0xB6: that is the answer to the "ready to connect" request
    // (see Write_FirstSetup), which AskAndWaitForLR10KForReadyToConnect() has just received and consumed.
    // The LR 10K sends it only once, so waiting for it again never succeeds (stuck at "6 %", then timeout).
    // The old code only got past this because it read the port from two threads at once and a spare
    // 0xB6 often ended up in the ring buffer. Skipping the step sends exactly the same bytes as before.
    if (!FirstSetupComplete.empty() && FirstSetupComplete.front().first == 0xB6 && FirstSetupComplete.front().second)
        FirstSetupComplete.erase(FirstSetupComplete.begin());

    if (!SendHEXWriteReadSequenceToLR10K(FirstSetupComplete))
    {
        mainGUIGlobals::showPopUp_ConnectionTimeout = true;
        return false;
    }

    return true;
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
