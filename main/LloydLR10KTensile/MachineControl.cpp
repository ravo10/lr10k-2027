/**

    @file      MachineControl.cpp
    @brief     Commands to the LR 10K (connect, setup, start/stop, movement), watchdog and reconnect.
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"

#include "LR10KController.h"

// -----------------------------------------------------------------------------
// ------------------------------ Local Functions ------------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Get the current time in string form. (readable)
    @retval  - 
**/
const char* getCurrentTimeString()
{
    // Current date/time based on current system
    time_t now = time(0);
    // Convert now to string form
    char* dt = ctime(&now);

    return dt;
}

// -----------------------------------------------------------------------------
// --------------------------- LR 10K Control System ---------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Try to connect from the LR 10K Tensile Machine.
    @retval             - 
**/
bool LR10KControlSystem::TryConnectingToLR10K()
{
    bool setupOneSuccess = _LR10KController.w_Setup1();
    if (!setupOneSuccess)
        return false;

    // -------------------------------------------------------------------------
    // ------------ Start to intrepret stuff from the LR 10K machine -----------
    // -------------------------------------------------------------------------

    // Start the interpreter (it stops and waits for one from an earlier connection first, so two threads never
    // split the incoming bytes). It turns the bytes into messages and InterpretMessage() into load/extension/status.
    {
        using namespace LR10KDataDeEncryptionSystem;
        m_pThreadReadLR10K_stopFlag.store(true);
        machine.StopInterpreter();

        m_pThreadReadLR10K_stopFlag.store(false); // Allow it to run BEFORE starting it
        machine.StartInterpreter(&InterpretMessage,
            []() { return !m_pThreadReadLR10K_stopFlag.load() && !m_pThreadReadCom_stopFlag.load() && !mainGUIGlobals::done; },
            []() { m_SerialBuffer.ClearAllData(); });
    }

    // Start the "is the LR 10K still answering" watchdog from now
    LR10KControlSystem::WatchdogReset();

    // Update the GUI if everything is successful
    LR10KAppGlobals::configurationDone              = true;
    LR10KAppGlobals::currentLiveStatusMessage       = "No";
    LR10KAppGlobals::currentWarnErrorStatusMessage  = "OK";

    PlaySoundEffect("buttonClick1");

    DockingApps::console.AddLog("Connected PC to LR 10K successfully!");
    LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Applying settings ..."));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // -------------------------------------------------------------------------
    // -------------------------- Send setup to LR 10K -------------------------
    // -------------------------------------------------------------------------

    // Create a new thread seperate from main thread
    LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::SendSetupToLR10K);
    // Let the thread run independently detached from main
    LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading->detach();

    return true;
}
/**
    @brief  Try to disconnect from the LR 10K Tensile Machine.
    @retval  - 
**/
bool LR10KControlSystem::TryDisconnectFromLR10K()
{
    if (LR10KAppGlobals::configurationDone && LR10KCOMSystem::CheckIfCOMIsConnected())
    {
        LR10KControlSystem::TellLR10KToStopSendingData(); // !! Important !!
        Sleep(300); // !! Important !!
        _LR10KController.w_Disconnect();
        Sleep(50);
    }

    // Reset GUI variabels
    LR10KAppGlobals::configurationDone              = false;
    LR10KDataDeEncryptionSystem::lr10kNotResponding.store(false);
    LR10KDataDeEncryptionSystem::watchdogProbeReplyPending.store(false);

    LR10KAppGlobals::sendingLiveDataFromLR10K       = false;
    LR10KAppGlobals::currentLiveStatusMessage       = "N/A";
    LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("N/A"));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
    LR10KAppGlobals::currentWarnErrorStatusMessage  = "N/A";

    // Zero point: forgotten; after reconnecting the first data package is the load zero, the distance starts at 0
    LR10KDataDeEncryptionSystem::machine.ResetDistance();
    LR10KDataDeEncryptionSystem::Measurement().RequestLoadZero(LR10KDataDeEncryptionSystem::machine.Readings());
    TabControl::LR10KOffsetExtension            = 0.0f;

    TabControl::LR10KTravelDistanceControl      = 0.0f;
    TabControl::LR10KTravelDistanceActionAfter  = 0;

    // Terminate thread
    LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag.store(true);
    LR10KDataDeEncryptionSystem::machine.StopInterpreter(); // Waits until it has finished (within ~20 ms)
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadConnectingToLR10K);
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading);

    // Clear all data
    LR10KDataDeEncryptionSystem::m_SerialBuffer.ClearAllData();
    LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("N/A"));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();

    DockingApps::console.AddLog("Disconnected PC from LR 10K successfully!");

    return true;
}
/**
    @brief Send setup for future test to LR 10K Tensile Machine.
**/
void LR10KControlSystem::SendTestSetupToLR10K()
{
    LR10KController _LR10KController;

    const float fSpeed      = LR10KAppGlobals::testing_speedAndPlotActual[0];
    const float fJogSpeed   = LR10KAppGlobals::testing_speedAndPlotActual[1];

    DockingApps::console.AddLog("Setting up the LR 10K with current settings ...");
    std::string str = "PC | Sending setup : Test Speed = % .3f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));
    str.append(" ...");
    DockingApps::console.AddLog(str.c_str(), fSpeed);
    str = "PC | Jog Speed = %.3f ";
    str.append(LR10KAppGlobals::GetUnitOption<const char*>(2));
    str.append(" ...");
    DockingApps::console.AddLog(str.c_str(), fJogSpeed);
    DockingApps::console.AddLog("PC | %s Test.", LR10KAppGlobals::tensionTesting ? "Tension" : "Compression");

    LR10KDataDeEncryptionSystem::warnErrorStatus_Reset();

    // Send setup 2
    if (_LR10KController.w_Setup2(fSpeed, fJogSpeed, LR10KAppGlobals::tensionTesting))
    {
        Sleep(150);
        if (LR10KAppGlobals::setZeroPointOnStarTest)
            LR10KControlSystem::SetZeroPoint();

        PlaySoundEffect("buttonClick1");

        DockingApps::console.AddLog("PC | Finished sending setup to LR 10K!");
    }
}

/**
    @brief Make LR 10K Tensile Machine start sending data.
**/
void LR10KControlSystem::TellLR10KToSendData()
{
    if (LR10KAppGlobals::configurationDone)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xC0);

        DockingApps::console.AddLog("PC | Telling LR 10K to start sending data");
        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
        DockingApps::console.AddLog("... From GUI.");

        LR10KAppGlobals::sendingLiveDataFromLR10K = true;
        LR10KAppGlobals::currentLiveStatusMessage = "Yes";

        PlaySoundEffect("buttonClick1");
    }
}
/**
    @brief Make LR 10K Tensile Machine stop sending data.
**/
void LR10KControlSystem::TellLR10KToStopSendingData()
{
    if (LR10KAppGlobals::configurationDone)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xC1);

        DockingApps::console.AddLog("PC | Telling LR 10K to stop sending data");
        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
        DockingApps::console.AddLog("... From GUI.");

        LR10KAppGlobals::sendingLiveDataFromLR10K = false;
        LR10KAppGlobals::currentLiveStatusMessage = "No";
    }
}
/**
    @brief Get the status report from LR 10K Tensile Machine.
**/
void LR10KControlSystem::GetCurrReportFromLR10K()
{
    if (LR10KAppGlobals::configurationDone)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xB6);

        LR10KCOMSystem::SendDataToCom(bufferMessage, LR10KDataDeEncryptionSystem::m_hSerial);
        DockingApps::console.AddLog("... From GUI.");
    }
}


// -----------------------------------------------------------------------------
// ------------------------- Watchdog and reconnecting -------------------------
// -----------------------------------------------------------------------------

namespace {
    constexpr long long watchdogSilenceMs       = 3000; //!< No byte for this long: ask the LR 10K for its status.
    constexpr long long watchdogProbeIntervalMs = 1500; //!< Time to wait for an answer before asking again.
    constexpr int       watchdogMaxProbes       = 2;    //!< Unanswered status requests before the LR 10K counts as not responding.

    int       watchdogProbesSent  = 0;
    long long watchdogLastProbeMs = 0;

    /**
        @brief Background part of LR10KControlSystem::StartReconnect().
    **/
    void ReconnectWorker(int comPort)
    {
        LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading_isLoading.store(true);
        DockingApps::console.AddLog("Reconnecting to LR 10K on COM%d ...", comPort);
        LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>("Reconnecting ..."));
        DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();

        // Close everything first. (Safe to call when parts are already closed, e.g. after "Connection lost".)
        LR10KControlSystem::TryDisconnectFromLR10K();
        LR10KCOMSystem::TryDisconnectingFromCurrCOMPort();
        Sleep(500); // Give a re-plugged USB adapter / the driver a moment

        if (!LR10KCOMSystem::TryConnectingToCOMPort(comPort))
        {
            DockingApps::console.AddLog("[error] Reconnect failed: COM%d could not be opened. Is the cable/adapter plugged in?", comPort);
            LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>("Reconnect failed"));
            DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
            LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading_isLoading.store(false);
            LR10KDataDeEncryptionSystem::reconnectInProgress.store(false);
            return;
        }

        // Same as "Connect PC" in the menu (shows the timeout pop up itself if the LR 10K does not answer)
        LR10KDataDeEncryptionSystem::ConnectingToLR10K();

        if (LR10KAppGlobals::configurationDone)
            DockingApps::console.AddLog("Reconnected to LR 10K on COM%d.", comPort);
        else
            DockingApps::console.AddLog("[error] Reconnect failed: COM%d opened, but the LR 10K did not answer.", comPort);

        LR10KDataDeEncryptionSystem::reconnectInProgress.store(false);
    }
}

/**
    @brief Restart the watchdog timers.
**/
void LR10KControlSystem::WatchdogReset()
{
    watchdogProbesSent  = 0;
    watchdogLastProbeMs = 0;
    LR10KDataDeEncryptionSystem::watchdogProbeReplyPending.store(false);
    LR10KDataDeEncryptionSystem::lastByteReceivedMs.store(LR10KDataDeEncryptionSystem::SteadyNowMs());
}
/**
    @brief  Detects an LR 10K that stopped answering. Call once per frame on the GUI thread.
    @details A native serial port gives no error when the machine is switched off or the cable is
             pulled, it just goes quiet. So: if no byte arrived for a while, ask for the status (0xB6).
             The LR 10K answers that even when it is not streaming live data (e.g. paused, or live data
             turned off), so a quiet but healthy machine does not raise an alarm. If several requests
             in a row get no answer, the "not responding" pop up is shown.
**/
void LR10KControlSystem::WatchdogTick()
{
    using namespace LR10KDataDeEncryptionSystem;

    // Waiting for the user to answer the pop up
    if (lr10kNotResponding.load())
        return;

    // Only watch a connected machine that is not busy with connect/setup/reconnect
    if (mainGUIGlobals::useDemoMode
        || !LR10KAppGlobals::configurationDone
        || !LR10KCOMSystem::CheckIfCOMIsConnected()
        || m_pThreadLR10KIsLoading_isLoading.load()
        || reconnectInProgress.load())
    {
        WatchdogReset();
        return;
    }

    const long long now = SteadyNowMs();
    const long long silence = now - lastByteReceivedMs.load();

    if (silence < watchdogSilenceMs)
    {
        watchdogProbesSent = 0; // Data is flowing (or a probe was answered)
        return;
    }
    if (now - watchdogLastProbeMs < watchdogProbeIntervalMs)
        return; // Still waiting for the answer to the last probe

    if (watchdogProbesSent < watchdogMaxProbes)
    {
        LR10KBufferMessage bufferMessage;
        bufferMessage.DataSetSingleBufferMessage(0xB6); // SEND STATUS

        watchdogProbeReplyPending.store(true);
        LR10KCOMSystem::SendDataToCom(bufferMessage, m_hSerial);
        watchdogProbesSent++;
        watchdogLastProbeMs = now;
        return;
    }

    watchdogProbeReplyPending.store(false);
    DockingApps::console.AddLog("[error] LR 10K has not answered for %lld s (status requested %d times).", silence / 1000, watchdogProbesSent);
    LR10KAppGlobals::currentStatusMessage.store(const_cast<char*>("Not responding"));
    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
    lr10kNotResponding.store(true);
}
/**
    @brief  True if there is a COM port to reconnect to and nothing else is connecting right now.
**/
bool LR10KControlSystem::CanReconnect()
{
    return LR10KAppGlobals::lastConnectedComPortNumber > -1
        && !mainGUIGlobals::useDemoMode
        && !LR10KDataDeEncryptionSystem::reconnectInProgress.load()
        && !LR10KDataDeEncryptionSystem::m_pThreadLR10KIsLoading_isLoading.load();
}
/**
    @brief  Disconnect, reopen the last COM port and connect to the LR 10K again (on a background thread).
    @retval false if a reconnect is not possible right now (see CanReconnect()).
**/
bool LR10KControlSystem::StartReconnect()
{
    if (!LR10KControlSystem::CanReconnect())
        return false;

    LR10KDataDeEncryptionSystem::reconnectInProgress.store(true);
    LR10KDataDeEncryptionSystem::lr10kNotResponding.store(false);

    const int comPort = LR10KAppGlobals::lastConnectedComPortNumber;
    LR10KDataDeEncryptionSystem::m_pThreadReconnecting = std::make_unique<std::thread>(ReconnectWorker, comPort);
    LR10KDataDeEncryptionSystem::m_pThreadReconnecting->detach();

    return true;
}

/**
    @brief  Sets a zero point.
    @retval  - 
**/
bool LR10KControlSystem::SetZeroPoint(TabControl::TabTestDataClass* tabData)
{
    if ((LR10KAppGlobals::configurationDone || mainGUIGlobals::useDemoMode) && (tabData == nullptr || !tabData->test_unfinished_test_started && !tabData->test_finished))
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_SetZero();

        DockingApps::console.AddLog("... From GUI.");

        // Load: the next data package from the machine becomes the zero (0 until then)
        LR10KDataDeEncryptionSystem::Measurement().RequestLoadZero(LR10KDataDeEncryptionSystem::machine.Readings());

        // Extension: count the machine's distance from 0 again (demo: restart the simulated extension)
        if (!mainGUIGlobals::useDemoModeLiveExtension)
        {
            LR10KDataDeEncryptionSystem::machine.ResetDistance();
            TabControl::LR10KOffsetExtension = 0.0f;
        }

        LR10KAppGlobals::zeroPointIsSet = true;

        DockingApps::console.AddLog("PC: Zero point is set");

        return true;
    }

    return false;
}
/**
    @brief  Start a tensile test.
    @retval  -
**/
bool LR10KControlSystem::StartTest(TabControl::TabTestDataClass* tabData)
{
    auto executeTest = [tabData]() {
        if ((LR10KAppGlobals::configurationDone || mainGUIGlobals::useDemoMode) && !tabData->test_finished && !tabData->test_unfinished_test_started && !tabData->test_running_test)
        {
            tabData->current_temp_preSaveLocation_StarTestNow = false;

            if (g_Extensiometer.record_test)
            {
                // Start recording
                g_Extensiometer.is_recording = true;
            }

            // A real test will never start when demo mode is activated
            if (mainGUIGlobals::useDemoMode && LR10KAppGlobals::setZeroPointOnStarTest)
                SetZeroPoint(tabData);
            else if (LR10KAppGlobals::configurationDone)
            {
                LR10KControlSystem::SendTestSetupToLR10K();
                _LR10KController.w_Go();
            }

            tabData->LR10KTensileData.StartTimeTest = std::chrono::steady_clock::now();
            tabData->LR10KTensileData.TimeData.clear(); // Make sure to clear old data from previous tests too!

            tabData->tension = LR10KAppGlobals::tensionTesting;

            // Store the specimen dimensions from the calculator (checked above) in this test. Without this,
            // a new tab kept its 0.01 mm placeholders unless "use global values on new tests" was on, so the
            // graph, the live readout and the Excel export divided by a 0.01 mm gauge length
            // (e.g. 4.7 mm travel shown as 47 000 % strain) while the calculated points looked right.
            DockingApps::calculator.SetLocalMaterialProperties(tabData, LR10KAppGlobals::areaOptionPicked,
                LR10KAppGlobals::calc_thickness, LR10KAppGlobals::calc_width, LR10KAppGlobals::calc_length, LR10KAppGlobals::calc_diameter);
            DockingApps::console.AddLog("Specimen for this test: gauge length %.3f, area %.3f (%s).", tabData->length, tabData->area,
                LR10KAppGlobals::GetUnitOption<const char*>(0));

            tabData->test_running_test = true;
            tabData->test_unfinished_test_started = true;

            TabControl::aTestIsStartedOnTabIndexId = TabControl::currActiveTabIndexId;

            tabData->oldLoadErrorValue = tabData->settings_speedAndPlot[2];

            // Get the current time at the start of the program
            TabControl::timestampForLastAddedRegressionPoint = std::chrono::high_resolution_clock::now();

            // Force set the current tab
            TabControl::forcedTabIndexId = TabControl::currActiveTabIndexId;

            PlaySoundEffect("StartTest");

            return true;
        }

        };

    // Reset status to default if it got stuck in e.g. an error
    if (!mainGUIGlobals::useDemoMode)
        LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_status((LPARAM)0x00);

    if (tabData == nullptr) return false;

    // Check if user has added gauge length, gauge thickness etc. for test
    bool userHasNotAddedDimensions = LR10KAppGlobals::calc_length == 0
        || (LR10KAppGlobals::areaOptionPicked == 0 && (LR10KAppGlobals::calc_width == 0 || LR10KAppGlobals::calc_thickness == 0))
        || (LR10KAppGlobals::areaOptionPicked == 1 && LR10KAppGlobals::calc_diameter == 0);

    // Show a message
    if (userHasNotAddedDimensions)
    {
        ImGui::OpenPopup(mainGUIGlobals::popUpID_DimensionsNotAdded);
        return false;
    }

    if (tabData->current_temp_preSaveLocation == "")
    {
        // Specify pre-save location GUI to a folder
        if (tabData->current_temp_preSaveLocation == "")
        {
            fileBrowserPreSaveLR10KTest.Open();
            tabData->current_temp_preSaveLocation_StarTestNow = true;
        }
    }
    else
    {
        // If you expect a 1-minute test at 50Hz, that's 3,000 points. Reserve 10,000 to be safe!
        tabData->LR10KTensileData.Data.reserve(10000);
        tabData->LR10KTensileData.TimeData.reserve(10000);

        return executeTest();
    }

    return false;
}
/**
    @brief  End and complete a tensile test before fracture.
    @retval  -
**/
bool LR10KControlSystem::EndTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (tabData->test_unfinished_test_started && !tabData->test_finished)
    {
        if (LR10KAppGlobals::configurationDone)
        {
            _LR10KController.w_EndTest();
        }

        // Set the smoothed data
        ImVector<ImVec2> clean_point_data = SmoothTensileData(tabData->LR10KTensileData.Data, tabData->LR10KTensileData.SmoothedDataWindowIndex);
        tabData->LR10KTensileData.SmoothedData = clean_point_data;

        // Snap the dots so they perfectly hug the new curve!
        SnapCrucialPointsToSmoothedCurve(tabData->plot_dataPoints, tabData->LR10KTensileData.SmoothedData);

        tabData->LR10KTensileData.TimeData.clear(); // Make sure to clear old data from previous tests too!
        tabData->LR10KTensileData.StartTimeTest = TimePoint();

        g_Extensiometer.is_recording = false;
        //LR10KAppGlobals::zeroPointIsSet = false;

        tabData->test_finished = true;
        tabData->test_unfinished_test_started = false;
        tabData->test_running_test = false;

        // Disable force on the the current tab
        TabControl::forcedTabIndexId = -1;

        // Clear recording
        g_Extensiometer.video_save_path = "";
        g_Extensiometer.record_test = false;

        PlaySoundEffect("EndTest");

        const char* dt = getCurrentTimeString();

        tabData->date = dt;
        TabControl::aTestIsStartedOnTabIndexId = -1; // Reset

        // Automated assignment of crucial points
        if (!tabData->CheckIfCrucialPointExsists(2)) // Will also be assigned at automatic break point detection to get this point first... (just for visuals)
            tabData->AssignPoint_UltimateTensileStrength(tabData);

        LR10KDataDeEncryptionSystem::warnErrorStatus_Reset();

        return true;
    }

    return false;
}
/**
    @brief  Pause a started tensile test.
    @retval  -
**/
bool LR10KControlSystem::PauseTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (tabData->test_running_test && tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_PauseTest();

        tabData->test_running_test = false;

        PlaySoundEffect("buttonClick0");

        return true;
    }

    return false;
}
/**
    @brief  Continue a paused tensile test.
    @retval  -
**/
bool LR10KControlSystem::ContinueTest(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (!tabData->test_running_test && tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_ContinueTest();

        tabData->test_running_test = true;

        PlaySoundEffect("buttonClick0");

        return true;
    }

    return false;
}
/**
    @brief  Return to zero point after a test is finished.
    @retval  -
**/
bool LR10KControlSystem::ReturnToZeroPoint(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if (!tabData->test_unfinished_test_started || tabData->test_finished)
    {
        if ((LR10KAppGlobals::configurationDone && !mainGUIGlobals::useDemoMode) || (LR10KAppGlobals::configurationDone && mainGUIGlobals::useDemoModeLiveExtension))
            _LR10KController.w_ReturnZero();
        DockingApps::console.AddLog("... From GUI.");

        PlaySoundEffect("buttonClick1");

        return true;
    }

    return false;
}

/**
    @brief  Stop the LR 10K Tensile Machine
    @retval  -
**/
bool LR10KControlSystem::Translation_STOP(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_Stop();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Move LR 10K Tensile Machine UP. Velocity is in mm/s.
    @param  tabData -
    @param  velocity - 
    @retval          - 
**/
bool LR10KControlSystem::Translation_UP(TabControl::TabTestDataClass* tabData, float velocity)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_Up();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Move LR 10K Tensile Machine DOWN. Velocity is in mm/s.
    @param  tabData -
    @param  velocity - 
    @retval          - 
**/
bool LR10KControlSystem::Translation_DOWN(TabControl::TabTestDataClass* tabData, float velocity)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_Down();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Move the LR 10K Tensile Machine fast upwards.
    @param  tabData -
    @retval  - 
**/
bool LR10KControlSystem::Translation_FASTUP_press(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_FastUpPress();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Stop the fast upwards movement of the LR 10K Tensile Machine (the same for up/down)
    @retval  -
**/
bool LR10KControlSystem::Translation_FASTUP_release()
{
    if (LR10KAppGlobals::configurationDone)
        _LR10KController.w_FastUpRelease();
    DockingApps::console.AddLog("... From GUI.");

    return true;
}
/**
    @brief  Move the LR 10K Tensile Machine fast downwards.
    @param  tabData -
    @retval  -
**/
bool LR10KControlSystem::Translation_FASTDOWN_press(TabControl::TabTestDataClass* tabData)
{
    if (tabData == nullptr) return false;

    if ((mainGUIGlobals::useDemoMode && mainGUIGlobals::useDemoModeLiveExtension) || !tabData->test_unfinished_test_started)
    {
        if (LR10KAppGlobals::configurationDone)
            _LR10KController.w_FastDownPress();
        DockingApps::console.AddLog("... From GUI.");

        return true;
    }

    return false;
}
/**
    @brief  Stop the fast upwards movement of the LR 10K Tensile Machine (the same for up/down)
    @retval  - 
**/
bool LR10KControlSystem::Translation_FASTDOWN_release()
{
    if (LR10KAppGlobals::configurationDone)
        _LR10KController.w_FastDownRelease();
    DockingApps::console.AddLog("... From GUI.");

    return true;
}
