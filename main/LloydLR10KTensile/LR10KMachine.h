/**

    @file      LR10KMachine.h
    @brief     The connection to one LR 10K: ring buffer, reader thread and interpreter thread.
    @details   Portable (no Windows or GUI code) and unit tested with a fake serial port
               (tests/machine_test.cpp).

               Data flow:
                 SerialPort --(SerialReader thread)--> CRingBuffer --(interpreter thread +
                 LR10KProtocol::MessageParser)--> message handler

               Both threads are owned by this object, so stopping them waits until they have really
               finished. The message handler is where the program turns messages into load, extension
               and status (see LR10KDataDeEncryptionSystem::InterpretMessage).

**/
#pragma once

#include "LR10KProtocol.h"
#include "RingBuffer.h"
#include "SerialPort.h"
#include "SerialReader.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

class LR10KMachine
{
public:
    using MessageHandler = std::function<void(const LR10KProtocol::Message&)>;

    static constexpr std::chrono::milliseconds interpreterWaitSlice{ 20 }; //!< How often the interpreter re-checks "keep running" while idle.

    LR10KMachine() = default;
    LR10KMachine(const LR10KMachine&) = delete;
    LR10KMachine& operator=(const LR10KMachine&) = delete;
    ~LR10KMachine()
    {
        StopInterpreter();
        StopReading();
    }

    //! The buffer between the reader and the interpreter (also read directly during the connect handshake).
    CRingBuffer& Buffer() { return m_buffer; }

    // -------------------------------------------------------------------------
    // Reading (port -> buffer)
    // -------------------------------------------------------------------------

    /**
        @brief  Start copying @p port into the buffer on the reader thread (see SerialReader).
        @param  keepRunning - Checked before every read; reading ends when it returns false.
    **/
    void StartReading(SerialPort& port, std::function<bool()> keepRunning, SerialReader::Callbacks callbacks = {})
    {
        m_port = &port;
        m_reader.Start(port, m_buffer, std::move(keepRunning), std::move(callbacks));
    }

    //! Stop the reader thread and wait until it has finished (about one port read timeout at most).
    void StopReading() { m_reader.Stop(); }

    bool IsReading() const { return m_reader.IsRunning(); }

    //! Write bytes to the port used by StartReading(). Returns the number of bytes written.
    std::size_t Send(const std::uint8_t* data, std::size_t size, std::uint32_t& errorCode)
    {
        if (m_port == nullptr)
        {
            errorCode = 1;
            return 0;
        }
        return m_port->Write(data, size, errorCode);
    }

    // -------------------------------------------------------------------------
    // Interpreting (buffer -> messages)
    // -------------------------------------------------------------------------

    /**
        @brief  Start the interpreter thread: reads the buffer, frames messages and calls @p onMessage for each.
        @param  keepRunning - Checked between bytes; the thread ends when it returns false.
        @param  onExit      - Called on the interpreter thread just before it ends.
        @details A previous interpreter is stopped and waited for first, so two interpreters never split the bytes.
    **/
    void StartInterpreter(MessageHandler onMessage, std::function<bool()> keepRunning, std::function<void()> onExit = {})
    {
        StopInterpreter();
        m_interpreterStop.store(false);
        m_interpreterRunning.store(true);
        m_interpreter = std::thread(&LR10KMachine::InterpreterLoop, this, std::move(onMessage), std::move(keepRunning), std::move(onExit));
    }

    //! Ask the interpreter to end soon (within ~20 ms). Does not wait. Safe from any thread.
    void RequestStopInterpreter() { m_interpreterStop.store(true); }

    //! Stop the interpreter and wait until it has finished. On the interpreter thread itself it only requests the stop.
    void StopInterpreter()
    {
        m_interpreterStop.store(true);
        if (!m_interpreter.joinable())
            return;
        if (m_interpreter.get_id() == std::this_thread::get_id())
        {
            m_interpreter.detach();
            return;
        }
        m_interpreter.join();
    }

    bool IsInterpreting() const { return m_interpreterRunning.load(); }

private:
    void InterpreterLoop(MessageHandler onMessage, std::function<bool()> keepRunning, std::function<void()> onExit)
    {
        auto shouldRun = [&] { return !m_interpreterStop.load() && (!keepRunning || keepRunning()); };

        LR10KProtocol::MessageParser parser;
        LR10KProtocol::Message message;
        std::uint8_t byte = 0;

        while (shouldRun())
        {
            if (!m_buffer.TryRead(byte, interpreterWaitSlice))
                continue; // Nothing yet: check the stop conditions again

            if (parser.Feed(byte, message) && onMessage)
                onMessage(message);
        }

        if (onExit)
            onExit();
        m_interpreterRunning.store(false);
    }

    CRingBuffer m_buffer;
    SerialReader m_reader;
    SerialPort* m_port = nullptr;

    std::thread m_interpreter;
    std::atomic<bool> m_interpreterStop{ true };
    std::atomic<bool> m_interpreterRunning{ false };
};
