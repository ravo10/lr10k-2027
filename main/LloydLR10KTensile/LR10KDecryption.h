/**

    @file      LR10KDecryption.h
    @brief     Pre-defines functions for the LR10KDecryption.cpp file.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      3.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/
#pragma once

#include <Windows.h>

#include <thread>
#include <memory> //!< Include the memory header for std::unique_ptr
#include <atomic> //!< Include the atomic header for std::atomic
#include <mutex>
#include <condition_variable>
#include <chrono>

typedef uint8_t byte;

/**
    @brief  Custom map functions - to reditect one range to another spesific range.
    @param  x       -
    @param  in_min  -
    @param  in_max  -
    @param  out_min -
    @param  out_max -
    @retval         -
**/
inline long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
    @brief  Get the current time since epoch in milliseconds.
    @retval  -
**/
inline uint64_t getCurrentTime()
{
    // Current date/time based on current system
    auto now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    return now;
}

/**

    @class   CRingBuffer
    @brief   Run this class only on a seperate thread to not stall the main thread.
    @details ~
    @note    Original logic by: Olaf Hallan Graven

**/
class CRingBuffer
{
private:
    static const size_t bufferSize{ 200 };  //!< The maximum ring buffer size, before it gets overwritten from index 0 again.
    byte m_uData[bufferSize];               //!< Container for the buffer data.
    size_t m_uRead;                         //!< Position of read data from the buffer data. Will match the the m_uWrite when finished reading.
    size_t m_uWrite;                        //!< Position of written data that will be sent to.
    bool m_bEmpty;                          //!< Tells if m_uRead == m_uWrite, then every data is read and ready for more data to be written if true.

    std::mutex m_Protect;                   //!< Used for locking/unlocking the thread to ensure that multiple threads or processes do not access shared resources simultaneously.

public:
    CRingBuffer();

    /**
        @brief  Write data to the buffer.
        @param  Element     -
        @param  NonBlocking -
        @retval             -
    **/
    bool Write(byte Element, bool NonBlocking = false);
    /**
        @brief  Read data from the buffer and return the output.
        @param  configDone -
        @retval            -
    **/
    byte Read(bool configDone = true);

    /**
        @brief  Check if there are any data available in the buffer.
        @retval  -
    **/
    bool DataAvailable();

    /**
        @brief  [Experimental] Read out all data to clear the buffer.
    **/
    void ReadOutAllData();
    /**
        @brief  [Experimental] Clear all data; reset everything as it was initialized.
    **/
    void ClearAllData();
};

/**
    @namespace LR10KDataDeEncryptionSystem
    @brief     Is used to decrypt and convert to HEX data from the LR10K Tensile Machine
                using a custom decryption pattern found by some teachers at USN.
**/
namespace LR10KDataDeEncryptionSystem {
    //! \note Used for reading data from LR10K to buffer, and often includes a "m_SerialBuffer" pointer to written to as a paramter. Contains COM connection information.
    inline extern HANDLE m_hSerial = INVALID_HANDLE_VALUE;
#if UNICODE
    //! \note Show the connected COM port as a world string. E.g. COM1.
    inline extern std::wstring ConnectedCOMPortName = L"";
#else
    //! \note Show the connected COM port as a string. E.g. COM1.
    inline extern std::string ConnectedCOMPortName = "";
#endif
    //! \note Used for reading data from LR10K to buffer, and often includes a "m_SerialBuffer" pointer to written to as a paramter. Contains COM data. (a ring buffer)
    static CRingBuffer m_SerialBuffer;

    inline extern std::unique_ptr<std::thread> m_pThreadReadLR10K = nullptr;    //!< The thread that contains a hooks that reads the data retrived from the LR 10K Tensile Machine
    inline extern std::unique_ptr<std::thread> m_pThreadReadCom = nullptr;      //!< (INPUT) The thread that reads data from the COM port.

    inline extern std::unique_ptr<std::thread> m_pThreadConnectingToLR10K   = nullptr;   //!< The thread that contains a hooks that connects to the LR 10K Tensile Machine
    inline extern std::unique_ptr<std::thread> m_pThreadLR10KIsLoading      = nullptr;   //!< The thread that contains a hooks that setups to the LR 10K Tensile Machine

    inline extern std::atomic<bool> m_pThreadReadLR10K_stopFlag(true);          //!< Need to stop the threads "while loop" before joining it. For reading LR 10K serial buffer data.
    inline extern std::atomic<bool> m_pThreadReadCom_stopFlag(true);            //!< Need to stop the threads "while loop" before joining it. For reading COM port data.
    inline extern std::atomic<bool> m_pThreadLR10KIsLoading_isLoading(false);   //!< Can disable the GUI, so no commands gets sent before setup is complete.

    inline extern std::atomic<bool> somethingUnexpectedHappend(false);  //!< If this is true, then the hooks will shut down and display a pop up.

    // -------------------------------------------------------------------------
    // ---------------------- Runs on seperate threads => ----------------------
    // -------------------------------------------------------------------------

    UINT __cdecl    ReadFromLR10KAndWriteToRingBuffer(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer);
    UINT __cdecl    ConnectingToLR10K();
    UINT __cdecl    SendSetupToLR10K();
    UINT __cdecl    InterpetWrittenRingBufferData(HANDLE m_hSerial, CRingBuffer* m_SerialBuffer);

    // -------------------------------------------------------------------------
    // ---------------------- <= Runs on seperate threads ----------------------
    // -------------------------------------------------------------------------

    bool            ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(int trackID, std::chrono::steady_clock::time_point start, bool configDone = true);
    bool            ThreadCanRun_InterpetWrittenRingBufferData();

    void            TerminateUniqeThread(std::unique_ptr<std::thread>& m_pThread, std::atomic<bool>* stopFlag = nullptr);

    float           InterpetWrittenRingBufferData_getLoad();
    float           InterpetWrittenRingBufferData_getExtension();

    std::string     InterpetWrittenRingBufferData_getNice_load();
    std::string     InterpetWrittenRingBufferData_getNice_extension();

    void    InterpetWrittenRingBufferData_WriteGlobals(LPARAM lParam, int action = 0);

    //! \brief Multiplier used for handling load data to show correct value. Compensates for overflow.
    //! \details It may be related to sensor data calibration, unit conversion, or some other custom data processing requirement.
    inline extern byte bMultiplier_load = { 1 };

    __int16 DecodingBits_Load(byte uData1, byte uData2);
    __int16 DecodingBits_Distance(byte uData1, byte uData2);
    void    DecodingBits_Extensometer(byte uData1, byte uData2, __int16* lParam);

    void    InterpetWrittenRingBufferData_dataPackage(byte uData1, CRingBuffer* m_SerialBuffer);
    void    InterpetWrittenRingBufferData_overflowMessage(byte uData1, CRingBuffer* m_SerialBuffer);
    void    InterpetWrittenRingBufferData_statusMessage(byte uData1, CRingBuffer* m_SerialBuffer);
    void    InterpetWrittenRingBufferData_warnErrorStatusMessage(byte uData1, CRingBuffer* m_SerialBuffer);
    void    InterpetWrittenRingBufferData_unkownData(byte uData1);

    void    InterpetWrittenRingBufferData_handling_load(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_extension(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_extensometerRange(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_overflow(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_status(LPARAM lParam);
    void    InterpetWrittenRingBufferData_handling_warnErrorStatus(LPARAM lParam);
    void    warnErrorStatus_Reset();
    void    InterpetWrittenRingBufferData_handling_unknownData(LPARAM lParam);

    // -------------------------------------------------------------------------
    // ----------------------- Custom converter algorithm ----------------------
    // -------------------------------------------------------------------------
    float convertAlgorithmForExtension(float value);
    int figureOutHowManyDecimalPlacesToShowAccuracy(float value);
    const char* figureOutHowManyDecimalPlacesToShowAccuracy_char(float value);
    float figureOutHowManyDecimalPlacesToShowAccuracy_float(float value);
}
