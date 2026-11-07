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
#include <vector>

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

    // -------------------------------------------------------------------------
    // Protocol steps (send something, wait for the LR 10K's answer in the buffer)
    // -------------------------------------------------------------------------

    //! Sends bytes; returns false if they could not (all) be sent. Lets the program log and report errors.
    using SendFunction = std::function<bool(const std::uint8_t* data, std::size_t size)>;

    //! Result of WaitForByte().
    enum class WaitResult
    {
        Received,        //!< The expected byte arrived.
        OtherBytesLimit, //!< maxOtherBytesWhileWaiting other bytes arrived first (counted as success, like the original code).
        Timeout,         //!< Nothing matching within the timeout: the LR 10K did not answer.
        Stopped          //!< keepRunning returned false while waiting.
    };

    //! The LR 10K repeats its last byte until it answers with a new one. After this many other bytes the wait counts as done.
    static constexpr int maxOtherBytesWhileWaiting = 2000;

    /**
        @brief  Wait until @p expected arrives in the buffer, skipping other bytes.
        @param  keepRunning - Checked about every 20 ms; the wait ends with Stopped when it returns false.
    **/
    WaitResult WaitForByte(std::uint8_t expected, std::chrono::milliseconds timeout, const std::function<bool()>& keepRunning = {})
    {
        int otherBytes = 0;
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!keepRunning || keepRunning())
        {
            if (otherBytes >= maxOtherBytesWhileWaiting)
                return WaitResult::OtherBytesLimit;

            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline)
                return WaitResult::Timeout;

            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            std::uint8_t byte = 0;
            if (!m_buffer.TryRead(byte, remaining < interpreterWaitSlice ? remaining : interpreterWaitSlice))
                continue;

            if (byte == expected)
                return WaitResult::Received;
            otherBytes++;
        }
        return WaitResult::Stopped;
    }

    //! Result of AskReadyToConnect().
    struct HandshakeResult
    {
        bool ready = false;          //!< The LR 10K answered 0xB6.
        int requests = 0;            //!< How many requests were sent.
        int bytesReceived = 0;       //!< All bytes seen while waiting (0 = nothing arrives at all).
        std::uint8_t lastByte = 0;   //!< The last byte seen.
    };

    /**
        @brief  The "ready to connect?" handshake: send 0xB6 until the LR 10K answers 0xB6.
        @details Clears the buffer first, then up to @p maxRequests times: send 0xB6, look at every byte
                 arriving within @p answerWindow, then pause (also after the answer, like the original code).
                 The bytes after the 0xB6 answer (e.g. the 0x00 of "B6 00") stay in the buffer.
    **/
    HandshakeResult AskReadyToConnect(const SendFunction& send, int maxRequests = 31,
        std::chrono::milliseconds answerWindow = std::chrono::milliseconds(100),
        std::chrono::milliseconds pause = std::chrono::milliseconds(30))
    {
        HandshakeResult result;
        m_buffer.ClearAllData(); // Only fresh answers count (e.g. not live data from an earlier session)

        const std::uint8_t request = LR10KProtocol::MsgStatus;
        while (!result.ready)
        {
            send(&request, 1);
            result.requests++;

            const auto until = std::chrono::steady_clock::now() + answerWindow;
            while (!result.ready)
            {
                const auto now = std::chrono::steady_clock::now();
                if (now >= until)
                    break;
                std::uint8_t byte = 0;
                if (!m_buffer.TryRead(byte, std::chrono::duration_cast<std::chrono::milliseconds>(until - now)))
                    break;
                result.bytesReceived++;
                result.lastByte = byte;
                result.ready = byte == LR10KProtocol::MsgStatus;
            }

            if (result.requests >= maxRequests)
                break;
            std::this_thread::sleep_for(pause);
        }
        return result;
    }

    //! How RunSequence() ended.
    enum class SequenceOutcome
    {
        Completed,           //!< Every expected reply arrived and the rest was sent.
        StoppedBeforeStep,   //!< keepRunning returned false between steps (e.g. the port was closed).
        NoReply,             //!< The LR 10K did not send an expected byte within the timeout.
        StoppedWhileWaiting, //!< keepRunning returned false while waiting for a reply.
        FinalSendFailed      //!< All replies arrived, but the last message could not be sent.
    };

    //! Result of RunSequence().
    struct SequenceResult
    {
        SequenceOutcome outcome = SequenceOutcome::Completed;
        int repliesExpected = 0;
        int repliesReceived = 0;
        std::uint8_t waitingFor = 0; //!< The byte that did not arrive (NoReply / StoppedWhileWaiting).
        bool Ok() const { return outcome == SequenceOutcome::Completed; }
    };

    /**
        @brief  Run a send/expect sequence such as LR10KSequences::FirstSetup().
        @details Exactly like the original LR10KController::SendHEXWriteReadSequenceToLR10K:
                 - (byte, false): add the byte to the message. The first byte after a reply starts a new message.
                 - (byte, true): wait @p preSendDelay, send the message built so far (again, if nothing was added
                   since the last reply), then wait for the byte. A failed send is not an error by itself.
                 - At the end, the message is sent once more if it is not empty.
        @param  onProgress - Called before waiting for each reply with (reply number, replies in total).
    **/
    SequenceResult RunSequence(const std::vector<std::pair<std::uint8_t, bool>>& sequence, const SendFunction& send,
        std::chrono::milliseconds replyTimeout, const std::function<bool()>& keepRunning = {},
        const std::function<void(int, int)>& onProgress = {},
        std::chrono::milliseconds preSendDelay = std::chrono::milliseconds(6))
    {
        SequenceResult result;
        for (const auto& step : sequence)
            if (step.second)
                result.repliesExpected++;

        std::vector<std::uint8_t> message;
        bool newMessageOnNextByte = false;

        for (const auto& step : sequence)
        {
            if (keepRunning && !keepRunning())
            {
                result.outcome = SequenceOutcome::StoppedBeforeStep;
                return result;
            }

            if (!step.second)
            {
                if (newMessageOnNextByte)
                    message.clear();
                message.push_back(step.first);
                newMessageOnNextByte = false;
                continue;
            }

            if (onProgress)
                onProgress(result.repliesReceived + 1, result.repliesExpected);

            std::this_thread::sleep_for(preSendDelay);
            if (!message.empty())
                send(message.data(), message.size());

            const WaitResult wait = WaitForByte(step.first, replyTimeout, keepRunning);
            if (wait == WaitResult::Timeout || wait == WaitResult::Stopped)
            {
                result.outcome = wait == WaitResult::Timeout ? SequenceOutcome::NoReply : SequenceOutcome::StoppedWhileWaiting;
                result.waitingFor = step.first;
                return result;
            }
            result.repliesReceived++;
            newMessageOnNextByte = true;
        }

        if (!message.empty() && !send(message.data(), message.size()))
            result.outcome = SequenceOutcome::FinalSendFailed;
        return result;
    }

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
