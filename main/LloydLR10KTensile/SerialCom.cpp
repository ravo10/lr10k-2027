/**

    @file      SerialCom.cpp
    @brief     Hardware I/O: COM port, reader/interpreter threads and decoding of LR 10K data.
    @details   Split out of common.cpp (code moved unchanged). Declarations are in common.h.
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/

#include "common.h"
#include "MaterialCalc.h"

#include "LR10KController.h"

#include <shellapi.h>   //!< ShellExecuteW (open the log folder)
#include <ctime>
#include <filesystem>

// -----------------------------------------------------------------------------
// ----------------------------- LR 10K COM System -----------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Find all COM ports available on the current computer (same numbers as in Device Manager).
    @retval  -
**/
std::vector<ULONG> LR10KCOMSystem::FindAllCOMPorts()
{
    return Win32SerialPort::ListPorts();
}
/**
    @brief  Connect to COM Port. When iPortNumber = -1, the first port available will be connected to.
    @details Opens the port (Win32SerialPort) and starts the reader thread (SerialReader) that copies
             incoming bytes into the ring buffer.
    @param  iPortNumber -
    @retval             -
**/
bool LR10KCOMSystem::TryConnectingToCOMPort(int iPortNumber)
{
    using namespace LR10KDataDeEncryptionSystem;

    // Maybe disconnect from previous COM port first
    LR10KCOMSystem::TryDisconnectingFromCurrCOMPort();

    std::vector<std::string> warnings;
    DWORD error = 0;
    int portNumber = iPortNumber;

    // Connect to a spesific COM Port
    if (portNumber > -1)
    {
        if (!comPort.Open(portNumber, warnings, error))
        {
            DockingApps::console.AddLog("[error] COM%d Port does not exist/could not be connected to (error %lu)...", portNumber, error);
            return false;
        }
    }
    else
    {
        // Check for maximum 100 ports - Try connecting to the first valid one
        for (int i = 0; i < 100 && !comPort.IsOpen(); i++)
        {
            warnings.clear();
            if (comPort.Open(i, warnings, error))
                portNumber = i;
        }

        // No valid serial port found on computer...
        if (!comPort.IsOpen())
        {
            DockingApps::console.AddLog("[error] No valid COM Port found on computer out of the first 100...");
            return false;
        }
    }

    // Settings the driver did not accept (the port is used anyway, like before)
    for (const std::string& warning : warnings)
        DockingApps::console.AddLog("[warning] COM%d: %s", portNumber, warning.c_str());

    m_hSerial = comPort.NativeHandle(); // For code that still uses the handle directly

    // Throw away anything left in the driver buffers from a previous session
    comPort.DiscardBuffers();
    m_SerialBuffer.ClearAllData();
    comConnectionLost.store(false);

    // ---------------------------------------------------
    // Start reading from COM
    // ---------------------------------------------------

    // Allow the reader to run BEFORE starting it (it checks this flag before every read)
    m_pThreadReadCom_stopFlag.store(false);

    SerialReader::Callbacks callbacks;
    callbacks.onByte = [](std::uint8_t byte)
    {
        // For the watchdog: when the LR 10K last sent something
        lastByteReceivedMs.store(SteadyNowMs());
        trafficLog.LogReceived(byte);
    };
    callbacks.onConnectionLost = [](std::uint32_t lastError, int failedReads)
    {
        // Handled on the GUI thread (main.cpp): disconnect and show the "Connection lost" pop up
        printf("COM read failed %d times (last error %lu). Connection lost.\n", failedReads, (unsigned long)lastError);
        trafficLog.LogNote("Connection lost: reading failed " + std::to_string(failedReads) + " times (error " + std::to_string(lastError) + ")");
        comConnectionLost.store(true);
    };
    callbacks.onExit = []()
    {
        // The interpreter depends on the reader: stop it too, and drop unread bytes
        m_pThreadReadLR10K_stopFlag.store(true);
        machine.RequestStopInterpreter();
        m_SerialBuffer.ClearAllData();
    };
    machine.StartReading(comPort,
        []() { return !m_pThreadReadCom_stopFlag.load() && !mainGUIGlobals::done; },
        callbacks);

    // Everything OK
    LR10KAppGlobals::connectedComPortNumber = portNumber; // (the "first available" search used to store -1 here)
    LR10KAppGlobals::lastConnectedComPortNumber = portNumber;

    // Update the GUI if everything is successful
    LR10KAppGlobals::readyToConnectToLR10K = true;

    PlaySoundEffect("buttonClick1");

    DockingApps::console.AddLog("COM%d port connected!", LR10KAppGlobals::connectedComPortNumber);
    trafficLog.LogNote("COM" + std::to_string(portNumber) + " opened (9600 baud 8N1)");

    return true;
}
/**
    @brief  Tries to disconnect from current COM port.
    @retval  -
**/
bool LR10KCOMSystem::TryDisconnectingFromCurrCOMPort()
{
    using namespace LR10KDataDeEncryptionSystem;

    LR10KControlSystem::TellLR10KToStopSendingData();
    Sleep(30);

    if (comPort.IsOpen() && LR10KCOMSystem::CheckIfCOMIsConnected())
    {
        // Stop the reader and wait until it has really finished (at most one read timeout, ~100 ms),
        // so it never reads from a closed port or into the next connection.
        m_pThreadReadCom_stopFlag.store(true);
        machine.StopReading();

        const int portNumber = LR10KAppGlobals::connectedComPortNumber;
        const bool closed = comPort.Close();
        m_hSerial = INVALID_HANDLE_VALUE;
        LR10KAppGlobals::connectedComPortNumber = -1;

        trafficLog.LogNote("COM" + std::to_string(portNumber) + " closed");
        if (closed)
            DockingApps::console.AddLog("Disconnected from COM%d...", portNumber);
        else
            DockingApps::console.AddLog("[error] Could not close COM%d properly (error %lu)...", portNumber, GetLastError());

        return true;
    }

    return false;
}
/**
    @brief  Checks if COM is connected to software or not.
    @retval  - 
**/
bool LR10KCOMSystem::CheckIfCOMIsConnected()
{
    return LR10KAppGlobals::connectedComPortNumber > -1;
}
/**
    @brief  Send data from GUI to LR 10K Tensile Machine.
    @param  bufferMessage -
    @param  hSerial       -
    @retval               -
**/
bool LR10KCOMSystem::SendDataToCom(LR10KBufferMessage bufferMessage, HANDLE /*hSerial: unused, the open port is used*/)
{
    if (bufferMessage.isBuffSingle())
        return LR10KCOMSystem::SendBytesToCom(&bufferMessage.message_single, 1);

    if (bufferMessage.size_of_message == 0)
        return true; // Nothing to send
    return LR10KCOMSystem::SendBytesToCom(bufferMessage.message, bufferMessage.size_of_message);
}
/**
    @brief  Send bytes to the LR 10K through the open COM port. Logs them ("Log serial traffic to file")
            and reports failed or incomplete writes in the console.
    @retval false if not all bytes could be sent.
**/
bool LR10KCOMSystem::SendBytesToCom(const std::uint8_t* data, std::size_t size)
{
    if (size == 0)
        return true;

    std::uint32_t error = 0;
    const std::size_t written = LR10KDataDeEncryptionSystem::comPort.Write(data, size, error);

    // "Log serial traffic to file": what was actually sent, and a note if not everything was
    LR10KDataDeEncryptionSystem::trafficLog.LogSent(data, written);
    if (written < size)
        LR10KDataDeEncryptionSystem::trafficLog.LogNote("Write incomplete: " + std::to_string(written) + " of " + std::to_string(size) + " bytes sent (error " + std::to_string(error) + ")");

    // If it fails to send data from COM
    if (written == 0)
    {
        DockingApps::console.AddLog("[error] Failed to send data to COM (error %lu)... Check connection!", (unsigned long)error);
        return false;
    }
    if (written < size)
    {
        // Write timed out part way (e.g. the LR 10K is holding CTS/DSR low)
        DockingApps::console.AddLog("[error] Only %lu of %lu bytes were sent to COM... Check connection!", (unsigned long)written, (unsigned long)size);
        return false;
    }

    return true;
}
// -----------------------------------------------------------------------------
// Serial traffic log ("Log serial traffic to file" in the menu)
// -----------------------------------------------------------------------------

namespace {
    //! Documents\LR10K serial logs (wide-character path, so user names with e.g. "ø" work).
    std::filesystem::path TrafficLogFolder()
    {
        std::filesystem::path folder;
        PWSTR documents = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, NULL, &documents) == S_OK)
            folder = std::filesystem::path(documents) / L"LR10K serial logs";
        if (documents)
            CoTaskMemFree(documents);
        return folder;
    }

    //! A path as UTF-8 text for the console (path::u8string() is char8_t in C++20).
    std::string PathText(const std::filesystem::path& path)
    {
        const auto text = path.u8string();
        return std::string(text.begin(), text.end());
    }

    std::string LocalTimeString(const char* format)
    {
        std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        char text[64];
        std::strftime(text, sizeof(text), format, &local);
        return text;
    }
}

bool LR10KCOMSystem::StartTrafficLog()
{
    const std::filesystem::path folder = TrafficLogFolder();
    if (folder.empty())
    {
        DockingApps::console.AddLog("[error] Serial traffic log: could not find the Documents folder.");
        return false;
    }

    const std::filesystem::path file = folder / ("serial_" + LocalTimeString("%Y-%m-%d_%H-%M-%S") + ".log");
    std::string description = "Started " + LocalTimeString("%Y-%m-%d %H:%M:%S") + ", app version " + LR10KAppVersion;
    if (LR10KCOMSystem::CheckIfCOMIsConnected())
        description += ", COM" + std::to_string(LR10KAppGlobals::connectedComPortNumber) + " 9600 baud 8N1";
    else
        description += ", no COM port open yet";

    if (!LR10KDataDeEncryptionSystem::trafficLog.Start(file, description))
    {
        DockingApps::console.AddLog("[error] Serial traffic log: could not create %s", PathText(file).c_str());
        return false;
    }

    DockingApps::console.AddLog("Logging serial traffic to %s", PathText(file).c_str());
    return true;
}

void LR10KCOMSystem::StopTrafficLog()
{
    if (!LR10KDataDeEncryptionSystem::trafficLog.IsActive())
        return;
    LR10KDataDeEncryptionSystem::trafficLog.Stop();
    DockingApps::console.AddLog("Stopped logging serial traffic (%s).", PathText(LR10KDataDeEncryptionSystem::trafficLog.Path()).c_str());
}

void LR10KCOMSystem::OpenTrafficLogFolder()
{
    const std::filesystem::path folder = TrafficLogFolder();
    if (folder.empty())
        return;
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    ShellExecuteW(NULL, L"open", folder.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
}

/**
    @brief  Reads data sent back from the LR 10K Tensile Machine through the COM port. It waits for a certain HEX feedback.
    @param  hexdecimalData -
    @retval                -
**/
bool LR10KCOMSystem::WaitAndReadFromCOM(byte hexdecimalData, std::chrono::milliseconds timeout)
{
    // The waiting itself is LR10KMachine::WaitForByte (tested in tests/machine_test.cpp / connect_replay_test.cpp)
    const auto result = LR10KDataDeEncryptionSystem::machine.WaitForByte(hexdecimalData, timeout,
        []() { return !LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag.load(); });

    if (result == LR10KMachine::WaitResult::Timeout)
        DockingApps::console.AddLog("[error] LR 10K did not answer (waited %lld ms for 0x%02X). Check the cable and that \"Remote control\" is on.",
            (long long)timeout.count(), hexdecimalData);

    // Received, or the LR 10K kept repeating other bytes (counted as done, like before)
    return result == LR10KMachine::WaitResult::Received || result == LR10KMachine::WaitResult::OtherBytesLimit;
}

// -----------------------------------------------------------------------------
// LR 10K Data Decryption/Encryption System (some code in "LR10KDecryption.cpp" also)
// -----------------------------------------------------------------------------

// (INPUT 1) Reading the COM port into the ring buffer is done by SerialReader (SerialReader.h),
// started in LR10KCOMSystem::TryConnectingToCOMPort().

/**
    @brief  Used for setup 1 process, so not everything freezes and the user can see the status of connection.
    @retval  - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::ConnectingToLR10K()
{
    m_pThreadLR10KIsLoading_isLoading.store(true);

        LR10KControlSystem::TryConnectingToLR10K();

    Sleep(150); // !!Important!!
    m_pThreadLR10KIsLoading_isLoading.store(false);

    return 0;
}

/**
    @brief  Used for setup 2 process, so not everything freezes and the user can see the status of connection.
    @retval  - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::SendSetupToLR10K()
{
    m_pThreadLR10KIsLoading_isLoading.store(true);

        LR10KControlSystem::SendTestSetupToLR10K();
        LR10KControlSystem::TellLR10KToSendData(); // Make it "official"

    Sleep(150); // !!Important!!
    m_pThreadLR10KIsLoading_isLoading.store(false);

    return 0;
}

// (INPUT 2) Interpreting the ring buffer is done by the interpreter thread in LR10KMachine (LR10KMachine.h):
// it frames the bytes into messages and calls LR10KDataDeEncryptionSystem::InterpretMessage() for each.

// -----------------------------------------------------------------------------
// ------------- Handle data coming from the LR 10K Tensile Machine ------------
// -----------------------------------------------------------------------------

double RoundTo(double val, int prec) {
    double p = pow(10, (double)prec);

    return floor(val * p + 0.5) / p;
}

/**
    @brief  Calculates readable load from raw load.
    @retval  - 
**/
float LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad()
{
    // double, not int: the old int truncated every reading to whole newtons (the sensor resolution is ~0.76 N)
    double value = ((double)TabControl::LR10KRawLoad - (double)TabControl::LR10KOffsetLoad) * LR10KAppGlobals::Convert_to_Newton;
    // Reverse positive direction when compression is enabled
    value = LR10KAppGlobals::tensionTesting ? value : value * -1;

    return (float)RoundTo((double)(value * LR10KAppGlobals::GetUnitOption<double>(9)), 3);
}
/**
    @brief  Calculates readable distance from raw values.
    @retval  -
**/
float LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension()
{
    float value;

    if (LR10KAppGlobals::opticalSourceAsExtensiometer)
        // Real extension from the optical camera source (comes from the Computer Vision thread)
        value = g_Extensiometer.results.travel_mm;
    else
        // From machine
        value = TabControl::LR10KRawExtension * LR10KAppGlobals::Convert_to_mm;

    // Reverse positive direction when compression is enabled
    value = LR10KAppGlobals::tensionTesting ? value : value * -1;

    return (float)RoundTo((double)(value * LR10KAppGlobals::GetUnitOption<double>(8)), 3);
}

/**
    @brief  Displays a nice name for the load. (rounded of with two decimals)
    @retval  - 
**/
std::string LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_load()
{
    char roundedValueChar[20];
    sprintf_s(roundedValueChar, "%.1f", InterpetWrittenRingBufferData_getLoad());

    std::string str = roundedValueChar;
    return str;
}
/**
    @brief  Displays a nice name for the distance. (rounded of with two decimals)
    @retval  - 
**/
std::string LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_extension()
{
    char roundedValueChar[20];
    sprintf_s(roundedValueChar, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(LR10KAppGlobals::maximumExtensionAllowedInsideLR10K), InterpetWrittenRingBufferData_getExtension());

    std::string str = roundedValueChar;
    return str;
}

// -----------------------------------------------------------------------------
// (Step 4) Final convertion of the interpeted values (runs on the seperate thread "InterpetWrittenRingBufferData")
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
    @brief Current load that is being applied. (Newton)
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_load(LPARAM lParam)
{
    TabControl::LR10KRawLoad = (short)lParam;
    if (TabControl::LR10KOffsetLoad == 0)
        TabControl::LR10KOffsetLoad = TabControl::LR10KRawLoad;

    TabControl::LR10KLoadNice = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_load();
}
/**
    @brief Total distance travelled from zero point. (mm)
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_extension(LPARAM lParam)
{
    // Reverse positive direction when compression is enabled
    TabControl::LR10KRawExtension += (__int16)lParam;

    TabControl::LR10KExtensionNice = LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getNice_extension();
}
/**
    @brief Total distance travelled. (mm) ?????? ***Not finished
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_extensometerRange(LPARAM lParam)
{
    TabControl::LR10KRawExtensometerRange = (short)lParam;

    char roundedValueChar[20];
    sprintf_s(roundedValueChar, LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char((float)TabControl::LR10KRawExtensometerRange), (float)TabControl::LR10KRawExtensometerRange);
    //	m_sTmp.Format(_T("LR10K: Extensometer = %02x in %d"), m_sCurrentExtension, m_sCurrentExtension - m_sOffsetExtension);
    //	m_CtrlLoglist.InsertString(0, m_sTmp.GetString());

    TabControl::LR10KExtensometerRangeNice = roundedValueChar;
}
/**
    @brief Current byte multiplier in use.
    @details Used for load - look at docs for "bMultiplier_load".
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_overflow(LPARAM lParam)
{
    printf("Input, LR 10K: Multiplier update (for load); new multiplier = %d\n", (short)lParam);
}
/**
    @brief Current status the LR 10K Tensile Machine has.
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_status(LPARAM lParam)
{
    // Answer to a watchdog probe: only proves the LR 10K is alive. Update the status text quietly,
    // without logging every few seconds and without the "ready after return to zero" side effect below.
    if (LR10KDataDeEncryptionSystem::watchdogProbeReplyPending.exchange(false))
    {
        const char* quietStatus = nullptr;
        switch ((byte)lParam)
        {
        case 0x00: quietStatus = "Ready"; break;
        case 0x40: quietStatus = "Stopped fast movement"; break;
        case 0x41: quietStatus = "Going up"; break;
        case 0x61: quietStatus = "Going down"; break;
        default: break; // Unknown: keep the current text
        }
        if (quietStatus != nullptr)
        {
            LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>(quietStatus));
            DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
        }
        return;
    }

    // Special cases
    if ((byte)lParam == 0x00)
    {
        // If this is sent by LR 10K and it is because it is ready after return to zero point,
        // accept this position as valid if it is 0 for load and extension
        if (LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getLoad() <= 0.005f && LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_getExtension() <= 0.005f)
            LR10KAppGlobals::zeroPointIsSet = true;
    }

    switch ((byte)lParam)
    {
    case 0x00:
        DockingApps::console.AddLog("Input, LR 10K: Status - Ready");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Ready"));
        break;
    case 0x40:
        DockingApps::console.AddLog("Input, LR 10K: Status - Stopped fast movement");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Stopped fast movement"));
        break;
    case 0x41:
        DockingApps::console.AddLog("Input, LR 10K: Status - Going up");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Going up"));
        break;
    case 0x61:
        DockingApps::console.AddLog("Input, LR 10K: Status - Going down");
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Going down"));
        break;

    default:
        printf("Input, LR 10K: Status - Unknown = %02x\n", (byte)lParam);
        DockingApps::console.AddLog("Input, LR 10K: Status - Unknown = %02x", (byte)lParam);
        LR10KAppGlobals::currentStatusMessage.store(const_cast <char*>("Unknown - Look in console"));
        break;
    }

    DockingApps::mutexControl.currentStatusMessageStatusChanged.notify_all();
}
/**
    @brief Current warning/error status the LR 10K Tensile Machine has.
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_warnErrorStatus(LPARAM lParam)
{
    switch ((byte)lParam)
    {
    case 0x00:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - OK");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "OK";
        break;
    case 0x0C:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status -  Extension range exceeded (error 12)");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Extension range exceeded (error 12)";
        // This warning and others will be reset when the user ends a test or sends a new setup sequence
        break;
    case 0x02:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Tension overload stop. Go DOWN only");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Tension overload stop. Go DOWN only";
        break;
    case 0x05:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Lower crosshead limit stop. Go UP only");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Lower crosshead limit stop. Go UP only";
        break;
    case 0x06:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Compression overload stop. Go UP only");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Compression overload stop. Go UP only";
        break;
    case 0x12:
        DockingApps::console.AddLog("Input, LR 10K: Warn/Error Status - Keyboard buffer overflow");
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Keyboard buffer overflow";
        break;

    default:
        printf("Input, LR 10K: Warn/Error Status - Unknown = %02x\n", (byte)lParam);
        DockingApps::console.AddLog("Input, LR 10K: Status - Unknown = %02x", (byte)lParam);
        LR10KAppGlobals::currentWarnErrorStatusMessage = "Unknown - Look in console";
        break;
    }
}
/**
    @brief Resets the warning received from LR 10K in the software. Sometimes it doesn't send a new update for errors.
**/
void LR10KDataDeEncryptionSystem::warnErrorStatus_Reset()
{
    InterpetWrittenRingBufferData_handling_warnErrorStatus((LPARAM)0x00);
    DockingApps::console.AddLog("... From software.");
}
/**
    @brief Unknow message coming from the LR 10K Tensile Machine.
    @param lParam - 
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_handling_unknownData(LPARAM lParam)
{
    printf("Input, LR 10K: Unknown HEX :: 0x%02X\n", (byte)lParam);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
/**
    @brief  Converts a value the user wants to have as an extension limit for the LR 10K, and makes a new value.
    @details Decoded algorithm from HEX values. This float value is at the end converted to a new float for LR 10K.
                It was to my notice that the value for extension got changed, but not to the value entered. I managed to
                map the algorithm. It is basically every value under 5 will become 10. After that it goes in steps of 2, where
                every value between a certain amount will be changed to another value. For instance:
                0 -> 5 will become 10. 5.5 -> 10 will become 20. 10.7 -> 20 will become 30. Here there is an increase of +10
                from the 1 to 2 step. Now there are two steps (i = 2) i % 2 = 0 and the last value will become increased by +20.
                So the new limit will now be 50 instead of 20. 21 -> 30 will become 50 and 31 -> 40 will become 60. (+10).
                The last limit will now have added +20 again, then the next limit becomes 80. 41 -> 50 will become 80. And so on.
                Why is this used? Just so that the LR 10K machine can stop if the user accidentally goes over limit.
    @param  value -
    @retval       -
**/
float LR10KDataDeEncryptionSystem::convertAlgorithmForExtension(float value)
{
    // Initialize
    int limit = 20;
    int switcher = 1;
    int multiplier = 1;

    float newValue = roundf(value);

    while (true)
    {
        if (newValue > 5)
        {
            // Found correct number
            if ((newValue + 10 * multiplier) <= limit)
            {
                newValue = limit;
                break;
            }

            // Increase every 2 step
            if (switcher % 2 == 0)
            {
                switcher = 1;
                limit += 20;
                multiplier++;
            }
            else
            {
                limit += 10;
                switcher++;
            }
        }
        else
        {
            newValue = 10;
            break;
        }
    }

    // Never exceed 1000 mm (which is new machines limit)
    if (newValue > 1000.0f)
        newValue = 1000.0f;

    return newValue;
}

/**
    @brief  Calculates how many decimal places the machine is currently going to show. The accuracy.
    @details Extension <= 5 = %0.3f | Extension < 60 mm = %0.2f | 60 < Extension < 600 = %0.1f | Extension > 600 = %0.0f.
    @param  value - Intenger which gives away how many decimal places that should be shown.
    @retval       - 
**/
int LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy(float value)
{
    if (value <= 5.0f)
        return 3;
    else if (value < 60.0f)
        return 2;
    else if (value < 600.0f)
        return 1;

    return 0;
}
/**
    @brief  Calculates how many decimal places the machine is currently going to show. The accuracy.
    @details Extension <= 5 = %0.3f | Extension < 60 mm = %0.2f | 60 < Extension < 600 = %0.1f | Extension > 600 = %0.0f.
    @param  value - 
    @retval       - Characthers that can be used as needed. E.g. %0.1f.
**/
const char* LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_char(float value)
{
    if (value <= 5.0f)
        return "%0.3f";
    else if (value < 60.0f)
        return "%0.2f";
    else if (value < 600.0f)
        return "%0.1f";

    return "%0.0f";
}
/**
    @brief  Calculates how many decimal places the machine is currently going to show. The accuracy.
    @details Extension <= 5 = %0.3f | Extension < 60 mm = %0.2f | 60 < Extension < 600 = %0.1f | Extension > 600 = %0.0f.
    @param  value -
    @retval       - A float value that will represent the accuracy. E.g. 0.01f
**/
float LR10KDataDeEncryptionSystem::figureOutHowManyDecimalPlacesToShowAccuracy_float(float value)
{
    if (value <= 5.0f)
        return 0.001f;
    else if (value < 60.0f)
        return 0.01f;
    else if (value < 600.0f)
        return 0.1f;

    return 0.0f;
}
