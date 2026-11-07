/**

    @file      Win32SerialPort.cpp
    @brief     The real COM port (Windows), configured for the LR 10K. See Win32SerialPort.h.
    @details   The settings are the ones the program has always used (moved here from
               LR10KCOMSystem::TryConnectingToCOMPort).

**/

#include "Win32SerialPort.h"

#include <algorithm>

std::vector<ULONG> Win32SerialPort::ListPorts()
{
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

bool Win32SerialPort::Open(int portNumber, std::vector<std::string>& warnings, DWORD& errorCode)
{
    Close();
    errorCode = 0;

    // "\\.\COMn" works for every port number (plain "COMn" only up to COM9).
    // The wide-character API is used explicitly because the build configurations differ
    // (Debug x64 is MultiByte, the others Unicode).
    const std::wstring path = L"\\\\.\\COM" + std::to_wstring(portNumber);
    m_handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        errorCode = GetLastError();
        return false;
    }
    m_portNumber = portNumber;

    // Basic settings for the COM port that are compatible with the LR 10K Tensile Machine
    DCB serialParams = {};
    serialParams.DCBlength = sizeof(serialParams);

    GetCommState(m_handle, &serialParams);
    serialParams.BaudRate = 9600;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    serialParams.fInX = false;
    serialParams.fOutX = false;
    serialParams.fOutxCtsFlow = true;
    serialParams.fOutxDsrFlow = true;
    serialParams.fDsrSensitivity = true;
    serialParams.fDtrControl = DTR_CONTROL_HANDSHAKE;
    serialParams.fRtsControl = RTS_CONTROL_HANDSHAKE;

    // Timeouts (ms): a one-byte read returns after at most ~100 ms without data
    COMMTIMEOUTS timeout = {};
    timeout.ReadIntervalTimeout = 50;
    timeout.ReadTotalTimeoutConstant = 50;
    timeout.ReadTotalTimeoutMultiplier = 50;
    timeout.WriteTotalTimeoutConstant = 50;
    timeout.WriteTotalTimeoutMultiplier = 10;

    // Report settings the port/driver rejects, but keep going like the original code did
    if (!SetCommState(m_handle, &serialParams))
        warnings.push_back("serial settings were not accepted by the driver (error " + std::to_string(GetLastError()) + "). Trying anyway.");
    if (!SetCommTimeouts(m_handle, &timeout))
        warnings.push_back("read/write timeouts were not accepted by the driver (error " + std::to_string(GetLastError()) + "). Trying anyway.");

    return true;
}

SerialPort::ReadStatus Win32SerialPort::ReadByte(std::uint8_t& out, std::uint32_t& errorCode)
{
    errorCode = 0;
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        errorCode = ERROR_INVALID_HANDLE;
        return ReadStatus::Error;
    }

    DWORD readBytes{ 0 };
    BYTE value{ 0 };
    if (!ReadFile(m_handle, &value, 1, &readBytes, NULL))
    {
        errorCode = GetLastError();
        if (errorCode == 0)
            errorCode = ERROR_GEN_FAILURE;

        // Clear line errors (framing/overrun etc.) so the port can keep working if this was a glitch
        DWORD commErrors{ 0 };
        ClearCommError(m_handle, &commErrors, NULL);
        return ReadStatus::Error;
    }

    if (readBytes != 1)
        return ReadStatus::Timeout;

    out = value;
    return ReadStatus::Byte;
}

std::size_t Win32SerialPort::Write(const std::uint8_t* data, std::size_t size, std::uint32_t& errorCode)
{
    errorCode = 0;
    if (size == 0)
        return 0;
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        errorCode = ERROR_INVALID_HANDLE;
        return 0;
    }

    DWORD written{ 0 };
    if (!WriteFile(m_handle, data, (DWORD)size, &written, NULL))
        errorCode = GetLastError();

    return written;
}

void Win32SerialPort::DiscardBuffers()
{
    if (m_handle != INVALID_HANDLE_VALUE)
        PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

bool Win32SerialPort::Close()
{
    if (m_handle == INVALID_HANDLE_VALUE)
        return true;

    const bool closed = CloseHandle(m_handle) != FALSE;
    m_handle = INVALID_HANDLE_VALUE;
    m_portNumber = -1;
    return closed;
}
