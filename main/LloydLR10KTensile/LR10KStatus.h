/**

    @file      LR10KStatus.h
    @brief     The connection state and status texts of the LR 10K that the GUI shows.
    @details   Portable (no Windows or GUI code), thread safe and unit tested (tests/status_test.cpp).

               Written by the connect/setup/reconnect threads and the interpreter thread, read by the GUI
               every frame. This replaces separate globals (configurationDone, connectedComPortNumber,
               readyToConnectToLR10K, sendingLiveDataFromLR10K, zeroPointIsSet, currentStatusMessage,
               currentLiveStatusMessage, currentWarnErrorStatusMessage): most were plain variables written
               on one thread and read on another, and changes that belong together (e.g. "disconnected":
               not connected, no live data, texts back to N/A) could be seen half done.

               Status texts must be string literals or other text that lives as long as the program
               (e.g. the tables in LR10KProtocol.h); only the pointer is stored.

**/
#pragma once

#include <mutex>

class LR10KStatus
{
public:
    //! Everything at one moment (read once per frame so all parts belong together).
    struct Snapshot
    {
        int comPort = -1;              //!< The open COM port, -1 if none
        int lastComPort = -1;          //!< The last COM port that was opened (for "Reconnect"), -1 if none
        bool machineConnected = false; //!< Connected to the LR 10K (handshake and setup #1 done)
        bool liveData = false;         //!< The LR 10K was told to send live data
        bool zeroPointSet = false;
        const char* status = "N/A";    //!< "LR 10K Status" text
        const char* warnError = "N/A"; //!< "LR 10K Warning/Error" text

        bool PortOpen() const { return comPort > -1; }
        //! "LR 10K Live" text: N/A when not connected, else Yes/No.
        const char* LiveText() const { return !machineConnected ? "N/A" : (liveData ? "Yes" : "No"); }
    };

    // ------------------------------------------------------------------ events

    //! A COM port was opened.
    void OnPortOpened(int port)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.comPort = port;
        m_state.lastComPort = port;
    }

    //! The COM port was closed (the last port is kept for "Reconnect").
    void OnPortClosed()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.comPort = -1;
    }

    //! Handshake and setup #1 done: connected, no live data yet, no warnings. (The status text is set separately.)
    void OnMachineConnected()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.machineConnected = true;
        m_state.liveData = false;
        m_state.warnError = "OK";
    }

    //! Disconnected from the LR 10K (the COM port may still be open): everything back to N/A.
    void OnMachineDisconnected()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.machineConnected = false;
        m_state.liveData = false;
        m_state.status = "N/A";
        m_state.warnError = "N/A";
    }

    void SetLiveData(bool on)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.liveData = on;
    }

    void SetZeroPointSet(bool set)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.zeroPointSet = set;
    }

    //! @param text - A string literal (or other text that lives as long as the program). nullptr is ignored.
    void SetStatus(const char* text)
    {
        if (text == nullptr) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.status = text;
    }

    //! @param text - A string literal (or other text that lives as long as the program). nullptr is ignored.
    void SetWarnError(const char* text)
    {
        if (text == nullptr) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.warnError = text;
    }

    // ----------------------------------------------------------------- queries

    Snapshot Get() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state;
    }

    bool IsMachineConnected() const { return Get().machineConnected; }
    bool IsPortOpen() const { return Get().PortOpen(); }
    int ComPort() const { return Get().comPort; }
    int LastComPort() const { return Get().lastComPort; }
    bool IsLiveData() const { return Get().liveData; }
    bool IsZeroPointSet() const { return Get().zeroPointSet; }
    const char* Status() const { return Get().status; }
    const char* WarnError() const { return Get().warnError; }

private:
    mutable std::mutex m_mutex;
    Snapshot m_state;
};
