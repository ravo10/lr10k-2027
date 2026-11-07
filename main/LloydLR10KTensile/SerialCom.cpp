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

// -----------------------------------------------------------------------------
// ----------------------------- LR 10K COM System -----------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Find all COM ports available on the current computer.
    @retval  -
**/
std::vector<ULONG> LR10KCOMSystem::FindAllCOMPorts()
{
    // Find all COM Serial Ports (the same numbers as in Device Manager), in ascending order.
    // Returned by value: this used to return a pointer to a local array, which no longer existed when the
    // menu read it, so the list showed random port numbers (e.g. COM4 missing).
    std::vector<ULONG> portNumbers(256);
    ULONG portsFound = 0;

    ULONG result = GetCommPorts(portNumbers.data(), (ULONG)portNumbers.size(), &portsFound);
    if (result == ERROR_MORE_DATA)
    {
        portNumbers.resize(portsFound);
        result = GetCommPorts(portNumbers.data(), (ULONG)portNumbers.size(), &portsFound);
    }
    if (result != ERROR_SUCCESS)
        portsFound = 0; // ERROR_FILE_NOT_FOUND = no COM ports on this PC

    portNumbers.resize((std::min)((size_t)portsFound, portNumbers.size()));
    std::sort(portNumbers.begin(), portNumbers.end());

    return portNumbers;
}
/**
    @brief  Connect to COM Port. When iPortNumber = -1, the first port available will be connected to.
    @param  iPortNumber -
    @retval             -
**/
bool LR10KCOMSystem::TryConnectingToCOMPort(int iPortNumber)
{
    // Maybe disconnect from previous COM port first
    LR10KCOMSystem::TryDisconnectingFromCurrCOMPort();

    #if UNICODE
        std::wstring str = L"\\\\.\\COM";
        str.append(std::to_wstring(iPortNumber));
        LR10KDataDeEncryptionSystem::ConnectedCOMPortName = str;

        // Used for trying to creating/connecting to a COM port.
        auto tryToConnect = [](LPCWSTR str) { LR10KDataDeEncryptionSystem::m_hSerial = CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); };
    #else
        std::string str = "\\\\.\\COM";
        str.append(std::to_string(iPortNumber));
        LR10KDataDeEncryptionSystem::ConnectedCOMPortName = str;

        // Used for trying to creating/connecting to a COM port.
        auto tryToConnect = [](LPCSTR str) { LR10KDataDeEncryptionSystem::m_hSerial = CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); };
    #endif

    // Connect to a spesific COM Port
    if (iPortNumber > -1)
    {
        tryToConnect(str.c_str());

        // Could not connect
        if (LR10KDataDeEncryptionSystem::m_hSerial == INVALID_HANDLE_VALUE)
        {
            DockingApps::console.AddLog("[error] COM%d Port does not exist/could not be connected to...", iPortNumber);
            return false;
        }
    }
    else
    {
        // Check for maximum 100 ports - Try connecting to the first valid one
        for (int i = { 0 }; i < 100; i++)
        {
#if UNICODE
            str = L"\\\\.\\COM";
            str.append(std::to_wstring(i));
#else
            str = "\\\\.\\COM";
            str.append(std::to_string(i));
#endif // !UNICODE

            tryToConnect(str.c_str());

            // Connection successful
            if (LR10KDataDeEncryptionSystem::m_hSerial != INVALID_HANDLE_VALUE)
                break;
        }

        // No valid serial port found on computer...
        if (LR10KDataDeEncryptionSystem::m_hSerial == INVALID_HANDLE_VALUE)
        {
            DockingApps::console.AddLog("[error] No valid COM Port found on computer out of the first 100...");
            return false;
        }
    }

    // Set some basic settings for the COM Port that is
    // compatible with the LR 10K Tensile Macine
    DCB serialParams = { 0 };
    serialParams.DCBlength = sizeof(serialParams);

    GetCommState(LR10KDataDeEncryptionSystem::m_hSerial, &serialParams);
    serialParams.BaudRate = 9600;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    serialParams.fInX = false;
    serialParams.fOutX = false;
    serialParams.fOutxCtsFlow = true;
    serialParams.fOutxDsrFlow = true;
    serialParams.fDsrSensitivity = true;

    serialParams.fDtrControl = DTR_CONTROL_HANDSHAKE; // DTR_CONTROL_ENABLE; // DTR_CONTROL_HANDSHAKE;
    serialParams.fRtsControl = RTS_CONTROL_HANDSHAKE; // RTS_CONTROL_ENABLE; // RTS_CONTROL_HANDSHAKE; (same value as DTR_CONTROL_HANDSHAKE, which was used here before)

    // Set timeouts (ms)
    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = 50;
    timeout.ReadTotalTimeoutConstant = 50;
    timeout.ReadTotalTimeoutMultiplier = 50;
    timeout.WriteTotalTimeoutConstant = 50;
    timeout.WriteTotalTimeoutMultiplier = 10;

    // Report settings the port/driver rejects, but keep going like the original code did:
    // some USB serial adapters reject individual settings and still work with the LR 10K.
    if (!SetCommState(LR10KDataDeEncryptionSystem::m_hSerial, &serialParams))
        DockingApps::console.AddLog("[warning] COM%d: serial settings were not accepted by the driver (error %lu). Trying anyway.", iPortNumber, GetLastError());
    if (!SetCommTimeouts(LR10KDataDeEncryptionSystem::m_hSerial, &timeout))
        DockingApps::console.AddLog("[warning] COM%d: read/write timeouts were not accepted by the driver (error %lu). Trying anyway.", iPortNumber, GetLastError());

    // Throw away anything left in the driver buffers from a previous session
    PurgeComm(LR10KDataDeEncryptionSystem::m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    LR10KDataDeEncryptionSystem::m_SerialBuffer.ClearAllData();
    LR10KDataDeEncryptionSystem::comConnectionLost.store(false);

    // ---------------------------------------------------
    // Start reading from COM
    // ---------------------------------------------------

    // Allow the thread to run BEFORE starting it. (Setting this after the thread was created let the
    // new thread see the old "stop" value and exit immediately, so no data was ever read.)
    LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag.store(false);
    LR10KDataDeEncryptionSystem::m_pThreadReadCom_running.store(true);
    // Create a new thread to handle information, so it doesn't stall the main thread (old thread being destroyed when disconnecting from old COM port)
    LR10KDataDeEncryptionSystem::m_pThreadReadCom = std::make_unique<std::thread>(LR10KDataDeEncryptionSystem::ReadFromLR10KAndWriteToRingBuffer, LR10KDataDeEncryptionSystem::m_hSerial, &LR10KDataDeEncryptionSystem::m_SerialBuffer);
    // Let the thread run independently detached from main
    LR10KDataDeEncryptionSystem::m_pThreadReadCom->detach();

    // Everything OK
    LR10KAppGlobals::connectedComPortNumber = iPortNumber;
    if (iPortNumber > -1)
        LR10KAppGlobals::lastConnectedComPortNumber = iPortNumber;

    // Update the GUI if everything is successful
    LR10KAppGlobals::readyToConnectToLR10K = true;

    PlaySoundEffect("buttonClick1");

    DockingApps::console.AddLog("COM%d port connected!", LR10KAppGlobals::connectedComPortNumber);

    return true;
}
/**
    @brief  Tries to disconnect from current COM port.
    @retval  -
**/
bool LR10KCOMSystem::TryDisconnectingFromCurrCOMPort()
{
    LR10KControlSystem::TellLR10KToStopSendingData();
    Sleep(30);

    if (LR10KDataDeEncryptionSystem::m_hSerial != INVALID_HANDLE_VALUE && LR10KCOMSystem::CheckIfCOMIsConnected())
    {
        // Terminate thread (since we are disconnecting from COM port)
        LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadCom, &LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag);
        // The thread is detached, so make sure it has really left ReadFile before the handle is closed.
        // Otherwise it can keep running into the next connection and read from it at the same time as the new thread.
        if (!LR10KDataDeEncryptionSystem::WaitForThreadExit(LR10KDataDeEncryptionSystem::m_pThreadReadCom_running))
            DockingApps::console.AddLog("[warning] COM reader thread did not stop in time.");

        // Get file name
        #if UNICODE
                std::wstring str = L"\\\\.\\COM";
                str.append(std::to_wstring(LR10KAppGlobals::connectedComPortNumber));
        #else
                std::string str = "\\\\.\\COM";
                str.append(std::to_string(LR10KAppGlobals::connectedComPortNumber));
        #endif

        // Close handle
        if (CloseHandle(LR10KDataDeEncryptionSystem::m_hSerial))
        {
            DeleteFile(LR10KDataDeEncryptionSystem::ConnectedCOMPortName.c_str());
            DockingApps::console.AddLog("Disconnected from COM%d...", LR10KAppGlobals::connectedComPortNumber);

            LR10KDataDeEncryptionSystem::m_hSerial = INVALID_HANDLE_VALUE;
            LR10KAppGlobals::connectedComPortNumber = -1;

            return true;
        }
        else
            DockingApps::console.AddLog("[error] Could not disconnected from COM%d...", LR10KAppGlobals::connectedComPortNumber);
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
    @brief  Check if there are any data being sent from LR 10K Tensile Machine.
    @param  hSerial -
    @param  uInfo   -
    @retval         -
**/
BOOL LR10KCOMSystem::PeekDataFromCom(HANDLE hSerial, byte& uInfo)
{
    DWORD readBytes{ 0 };
    return ReadFile(hSerial, &uInfo, 1, &readBytes, NULL);
}
/**
    @brief  Read one byte from the COM port, waiting at most the port's read timeout (~100 ms).
    @param  hSerial - Open COM port.
    @param  uInfo   - Receives the byte when one was read.
    @param  error   - 0 on success or plain timeout; otherwise the Windows error code from ReadFile.
    @retval true if a byte was read.
**/
bool LR10KCOMSystem::ReadByteFromCom(HANDLE hSerial, byte& uInfo, DWORD& error)
{
    DWORD readBytes{ 0 };
    error = 0;

    if (!ReadFile(hSerial, &uInfo, 1, &readBytes, NULL))
    {
        error = GetLastError();
        if (error == 0)
            error = ERROR_GEN_FAILURE;

        // Clear line errors (framing/overrun etc.) so the port can keep working if this was a glitch
        DWORD commErrors{ 0 };
        ClearCommError(hSerial, &commErrors, NULL);
        return false;
    }

    return readBytes == 1;
}
/**
    @brief  Get full data from LR 10K Tensile Machine to GUI. Waits until a byte arrives,
            the COM thread is stopped, or reading fails.
    @param  hSerial -
    @retval The byte read, or 0x00 if nothing was read.
**/
byte LR10KCOMSystem::GetDataFromCom(HANDLE hSerial)
{
    byte uInfo = 0x00;
    DWORD error{ 0 };

    // Wait until data from COM is read
    const auto start = std::chrono::steady_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(6, start))
    {
        if (ReadByteFromCom(hSerial, uInfo, error))
            return uInfo;
        if (error != 0)
            break;
    }

    return 0x00;
}
/**
    @brief  Send data from GUI to LR 10K Tensile Machine.
    @param  bufferMessage -
    @param  hSerial       -
    @retval               -
**/
bool LR10KCOMSystem::SendDataToCom(LR10KBufferMessage bufferMessage, HANDLE hSerial)
{
    DWORD WriteBytes{ 0 }; // Was uninitialized: if WriteFile failed, the check below read garbage
    BOOL success;
    DWORD expectedBytes;

    if (bufferMessage.isBuffSingle())
    {
        expectedBytes = 1;
        success = WriteFile(hSerial, &bufferMessage.message_single, expectedBytes, &WriteBytes, NULL);
    }
    else
    {
        expectedBytes = (DWORD)bufferMessage.size_of_message;
        if (expectedBytes == 0)
            return true; // Nothing to send

        success = WriteFile(hSerial, &bufferMessage.message, expectedBytes, &WriteBytes, NULL);
    }

    // If it fails to send data from COM
    if (!success || WriteBytes == 0)
    {
        DockingApps::console.AddLog("[error] Failed to send data to COM (error %lu)... Check connection!", success ? 0UL : GetLastError());
        return false;
    }
    if (WriteBytes < expectedBytes)
    {
        // Write timed out part way (e.g. the LR 10K is holding CTS/DSR low)
        DockingApps::console.AddLog("[error] Only %lu of %lu bytes were sent to COM... Check connection!", WriteBytes, expectedBytes);
        return false;
    }

    return true;
}
/**
    @brief  Reads data sent back from the LR 10K Tensile Machine through the COM port. It waits for a certain HEX feedback.
    @param  hexdecimalData -
    @retval                -
**/
bool LR10KCOMSystem::WaitAndReadFromCOM(byte hexdecimalData, std::chrono::milliseconds timeout)
{
    byte rec{ 0 }; // Used to record packages of HEX data read from the serial buffer.

    // Try and read data
    int numberOfTries = 0;
    const auto start = std::chrono::steady_clock::now();
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(1, start))
    {
        // The LR 10K repeats its last byte until it answers with a new one. After this many
        // other bytes, continue anyway (same behaviour as before).
        if (numberOfTries >= 2000)
            return true;

        // No byte at all from the LR 10K for too long: it is not answering
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            DockingApps::console.AddLog("[error] LR 10K did not answer (waited %lld ms for 0x%02X). Check the cable and that \"Remote control\" is on.",
                (long long)timeout.count(), hexdecimalData);
            return false;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!LR10KDataDeEncryptionSystem::m_SerialBuffer.TryRead(rec, (std::min)(remaining, std::chrono::milliseconds(20))))
            continue;

        printf("Leser av COM: %02x\n", rec);
        if (rec == hexdecimalData)
            return true;

        numberOfTries++;
    }

    return false; // COM thread was stopped (disconnected) while waiting
}

// -----------------------------------------------------------------------------
// LR 10K Data Decryption/Encryption System (some code in "LR10KDecryption.cpp" also)
// -----------------------------------------------------------------------------

/**
    @brief  (INPUT 1) Write data from the LR 10K Tensile Machine to the round buffer. (runs on seperate thread)
    @param  m_hSerial      - 
    @param  m_SerialBuffer - 
    @retval                - 
**/
UINT __cdecl LR10KDataDeEncryptionSystem::ReadFromLR10KAndWriteToRingBuffer(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer)
{
    // A few failed reads in a row means the port is gone (USB adapter unplugged, driver reset, ...).
    // Before, ReadFile errors were ignored and this loop spun at 100% CPU forever without telling anyone.
    constexpr int maxConsecutiveReadErrors = 3;
    int consecutiveReadErrors = 0;

    byte Data;
    DWORD error{ 0 };
    const auto start = std::chrono::steady_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(2, start) && !mainGUIGlobals::done)
    {
        if (LR10KCOMSystem::ReadByteFromCom(m_hSerial, Data, error))
        {
            consecutiveReadErrors = 0;
            LR10KDataDeEncryptionSystem::lastByteReceivedMs.store(LR10KDataDeEncryptionSystem::SteadyNowMs());
            m_SerialBuffer->Write(Data);
            continue;
        }

        if (error == 0)
        {
            // Plain timeout: no data right now
            consecutiveReadErrors = 0;
            continue;
        }

        if (++consecutiveReadErrors >= maxConsecutiveReadErrors)
        {
            // Only report it if nobody asked this thread to stop (a normal disconnect also makes ReadFile fail)
            if (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(2, start))
            {
                printf("COM read failed %d times (last error %lu). Connection lost.\n", consecutiveReadErrors, error);
                LR10KDataDeEncryptionSystem::comConnectionLost.store(true);
            }
            break;
        }
        Sleep(50);
    }

    // Clean up
    // Terminate thread for (INPUT 2) - Not needed because it is dependent on this thread (INPUT 1)
    LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadLR10K, &LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag); //!<

    // Clear/reset serial buffer
    m_SerialBuffer->ClearAllData();

    LR10KDataDeEncryptionSystem::m_pThreadReadCom_running.store(false);
    return 0;
}

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

/**
    @brief  (INPUT 2) (Step 1) Interpet data from the LR10K Tensile Machine that has been stored in the ring buffer. (runs on seperate thread)
    @param  m_hSerial      - 
    @param  m_SerialBuffer - 
    @retval                - 
**/
byte LR10KDataDeEncryptionSystem::ReadNextByteForInterpreter(CRingBuffer* m_SerialBuffer)
{
    // Like CRingBuffer::Read(), but also gives up when the interpreter itself is told to stop.
    // Before, a stopped interpreter could stay blocked waiting for data, and after a reconnect two
    // interpreter threads would split the incoming bytes between them (garbled load/extension values).
    byte value = 0x00;
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_InterpetWrittenRingBufferData()
        && !LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag
        && !mainGUIGlobals::done)
    {
        if (m_SerialBuffer->TryRead(value, std::chrono::milliseconds(20)))
            return value;
    }

    return 0x00;
}

UINT __cdecl LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer)
{
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_InterpetWrittenRingBufferData() && !mainGUIGlobals::done)
    {
        byte uData1 = ReadNextByteForInterpreter(m_SerialBuffer);
        if (!LR10KDataDeEncryptionSystem::ThreadCanRun_InterpetWrittenRingBufferData() || LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag)
            break; // Stopped while waiting: there is no real byte to interpret
        // printf("Thread got some data from LR 10K: %02x ...\n", uData1);

        if (uData1 >= 0xE0)
            // We have a data package comming. (used for load, distance etc.)
            LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_dataPackage(uData1, m_SerialBuffer);
        else
        {
            switch (uData1)
            {
            case 0xB1:
                // We have a overflow message. (for getting the mutiplier)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_overflowMessage(uData1, m_SerialBuffer);
                break;
            case 0xB6:
                // We have a status message. (for getting the current status of LR 10K)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_statusMessage(uData1, m_SerialBuffer);
                break;
            case 0xBF:
                // We have a warning/error message. (used for e.g. magnetic stop)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_warnErrorStatusMessage(uData1, m_SerialBuffer);
                break;

            default:
                // Unknown. (as of now unkown data)
                LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_unkownData(uData1);
                break;
            }
        }
    }

    // Clear/reset serial buffer
    m_SerialBuffer->ClearAllData();

    LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_running.store(false);
    return 0;
}

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
