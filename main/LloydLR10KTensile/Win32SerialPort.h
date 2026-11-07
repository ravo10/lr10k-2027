/**

    @file      Win32SerialPort.h
    @brief     The real COM port (Windows), configured for the LR 10K.
    @details   9600 baud, 8 data bits, no parity, 1 stop bit, CTS/DSR hardware handshake,
               ~100 ms read timeout per byte. Settings the driver rejects are reported as warnings
               and the port is used anyway (some USB adapters reject single settings and still work).

**/
#pragma once

#include "SerialPort.h"

#include <Windows.h>

#include <string>
#include <vector>

class Win32SerialPort : public SerialPort
{
public:
    Win32SerialPort() = default;
    Win32SerialPort(const Win32SerialPort&) = delete;
    Win32SerialPort& operator=(const Win32SerialPort&) = delete;
    ~Win32SerialPort() override { Close(); }

    //! COM port numbers present on this PC (same numbers as in Device Manager), ascending.
    static std::vector<ULONG> ListPorts();

    /**
        @brief  Open COM<portNumber> with the LR 10K settings. Closes a previously open port first.
        @param  warnings  - Receives a text for every setting the driver did not accept.
        @param  errorCode - The Windows error code if opening failed, else 0.
        @retval false if the port could not be opened at all.
    **/
    bool Open(int portNumber, std::vector<std::string>& warnings, DWORD& errorCode);

    bool IsOpen() const override { return m_handle != INVALID_HANDLE_VALUE; }
    ReadStatus ReadByte(std::uint8_t& out, std::uint32_t& errorCode) override;
    std::size_t Write(const std::uint8_t* data, std::size_t size, std::uint32_t& errorCode) override;
    void DiscardBuffers() override;
    bool Close() override;

    //! The COM port number, or -1 when closed.
    int PortNumber() const { return m_portNumber; }
    //! The Windows handle (for code that still needs it), or INVALID_HANDLE_VALUE.
    HANDLE NativeHandle() const { return m_handle; }

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    int m_portNumber = -1;
};
