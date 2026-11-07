/**

    @file      SerialPort.h
    @brief     Abstract serial port: what the rest of the program needs from a COM port.
    @details   Portable (no Windows headers). The real port is Win32SerialPort; tests use a fake
               port (tests/serial_reader_test.cpp) that can replay scripted or recorded bytes.

**/
#pragma once

#include <cstddef>
#include <cstdint>

class SerialPort
{
public:
    //! Result of SerialPort::ReadByte().
    enum class ReadStatus
    {
        Byte,    //!< One byte was read.
        Timeout, //!< No byte within the port's read timeout (normal when the machine is quiet).
        Error    //!< The read failed (e.g. USB adapter unplugged); see the error code.
    };

    virtual ~SerialPort() = default;

    //! True while the port is open.
    virtual bool IsOpen() const = 0;

    /**
        @brief  Wait for one byte, at most the port's read timeout (~100 ms for the LR 10K settings).
        @param  out       - Receives the byte when ReadStatus::Byte is returned.
        @param  errorCode - Set to the system error code when ReadStatus::Error is returned, else 0.
    **/
    virtual ReadStatus ReadByte(std::uint8_t& out, std::uint32_t& errorCode) = 0;

    /**
        @brief  Write bytes.
        @param  errorCode - Set to the system error code if the write failed, else 0.
        @retval The number of bytes actually written (less than @p size on a timeout or error).
    **/
    virtual std::size_t Write(const std::uint8_t* data, std::size_t size, std::uint32_t& errorCode) = 0;

    //! Throw away anything waiting in the driver's receive and transmit buffers.
    virtual void DiscardBuffers() = 0;

    //! Close the port. Safe to call when already closed. Returns false if closing failed.
    virtual bool Close() = 0;
};
