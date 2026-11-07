/**

    @file      LR10KDecryption.cpp
    @brief     Contains logic for the LR 10K Data Decryption/Encryption System and
                Decodeing of the LR 10K Tensile Machine data.
    @details   ~
    @author    Rasmus Ullestad Relling
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/



#include "LR10KDecryption.h"
#include "LR10KProtocol.h" //!< The decoding itself (unit tested in tests/protocol_test.cpp)

// -----------------------------------------------------------------------------
// ------------------ LR 10K Data Decryption/Encryption System -----------------
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ----------------------------------- Decode ----------------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Decoder for the load.
    @details A multiplier is used to calculate when cirka above 1600 N.
    @param  uData1 -
    @param  uData2 -
    @retval        -
**/
__int16 LR10KDataDeEncryptionSystem::DecodingBits_Load(byte uData1, byte uData2)
{
    return LR10KProtocol::DecodeLoad(uData1, uData2, LR10KDataDeEncryptionSystem::bMultiplier_load);
}
/**
    @brief  Decoder for the distance.
    @param  uData1 -
    @param  uData2 -
    @retval        -
**/
__int16 LR10KDataDeEncryptionSystem::DecodingBits_Distance(byte uData1, byte uData2)
{
    return LR10KProtocol::DecodeDistanceDelta(uData1, uData2);
}
/**
    @brief Decoder for the extensometer.
    @param uData1 -
    @param uData2 -
    @param lParam -
**/
void LR10KDataDeEncryptionSystem::DecodingBits_Extensometer(byte uData1, byte uData2, __int16* lParam)
{
    *lParam = LR10KProtocol::DecodeExtensometer(uData1, uData2);
}

/**
    @brief (Step 2) (runs on a seperate thread; on "InterpetWrittenRingBufferData")
            For handling the load, distance and extensiometer values. These needs to be
            multiplied by a constant in namespace "LR10KAppGlobals" for readability.
    @param uData1         -
    @param m_SerialBuffer -
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_dataPackage(byte uData1, CRingBuffer* m_SerialBuffer)
{
    __int16 lParam;

    // Second data element
    byte uData2 = ReadNextByteForInterpreter(m_SerialBuffer);

    // Load
    lParam = LR10KDataDeEncryptionSystem::DecodingBits_Load(uData1, uData2);
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)lParam, 1);

    // Distance
    uData1 = ReadNextByteForInterpreter(m_SerialBuffer);
    uData2 = ReadNextByteForInterpreter(m_SerialBuffer);
    lParam = LR10KDataDeEncryptionSystem::DecodingBits_Distance(uData1, uData2);
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)lParam, 2);

    // Read extensometer
    uData1 = ReadNextByteForInterpreter(m_SerialBuffer);
    uData2 = ReadNextByteForInterpreter(m_SerialBuffer);
    LR10KDataDeEncryptionSystem::DecodingBits_Extensometer(uData1, uData2, &lParam);
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)lParam, 3);
}

/**
    @brief For handling the overflow message - that is the multiplier for the bytes.
    @param uData1         -
    @param m_SerialBuffer -
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_overflowMessage(byte uData1, CRingBuffer* m_SerialBuffer)
{
    LPARAM lParam = (LPARAM)ReadNextByteForInterpreter(m_SerialBuffer);
    LR10KDataDeEncryptionSystem::bMultiplier_load = LR10KProtocol::LoadMultiplierFromOverflow((byte)lParam);

    // Post multiplier to main.
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)LR10KDataDeEncryptionSystem::bMultiplier_load, 4);
}
/**
    @brief For handling the actual status. (ready, up, down, fast up etc.)
    @param uData1         -
    @param m_SerialBuffer -
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_statusMessage(byte uData1, CRingBuffer* m_SerialBuffer)
{
    LPARAM lParam = (LPARAM)ReadNextByteForInterpreter(m_SerialBuffer);
    // printf("ID_LR10K_STATUS %x\n", (int)lParam);

    // Post status to main.
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)lParam, 5);
}
/**
    @brief For handling the actual warning/error status. (ready, up, down, fast up etc.)
    @param uData1         -
    @param m_SerialBuffer -
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_warnErrorStatusMessage(byte uData1, CRingBuffer* m_SerialBuffer)
{
    LPARAM lParam = (LPARAM)ReadNextByteForInterpreter(m_SerialBuffer);
    // printf("ID_LR10K_WARN_ERROR_STATUS %x\n", (int)lParam);

    // Post status to main.
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)lParam, 6);
}
/**
    @brief For handling unknown data.
    @param uData1 -
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_unkownData(byte uData1)
{
    // Post data to main.
    InterpetWrittenRingBufferData_WriteGlobals((LPARAM)uData1);
}

/**
    @brief (Step 3) For posting values interpeted to user on main thread. (write global values)
            Send message to main thread with some new data, by writing to global values that e.g. the GUI can display
    @param lParam -
    @param action -
**/
void LR10KDataDeEncryptionSystem::InterpetWrittenRingBufferData_WriteGlobals(LPARAM lParam, int action)
{
    switch (action)
    {
    case 1:
        // Load
        InterpetWrittenRingBufferData_handling_load(lParam);
        break;
    case 2:
        // Distance
        InterpetWrittenRingBufferData_handling_extension(lParam);
        break;
    case 3:
        // Extensiometer
        InterpetWrittenRingBufferData_handling_extensometerRange(lParam);
        break;
    case 4:
        // Multiplier
        InterpetWrittenRingBufferData_handling_overflow(lParam);
        break;
    case 5:
        // Status
        InterpetWrittenRingBufferData_handling_status(lParam);
        break;
    case 6:
        // Warning/Error Status
        InterpetWrittenRingBufferData_handling_warnErrorStatus(lParam);
        break;

    default:
        // Unknown
        InterpetWrittenRingBufferData_handling_unknownData(lParam);
        break;
    }
}

// -----------------------------------------------------------------------------
// ------------------------------- Thread control ------------------------------
// -----------------------------------------------------------------------------

/**
    @brief  Will be executed if threads hooks are allowed to continue running.
    @param  trackID    -
    @param  start      -
    @param  configDone -
    @retval            -
**/
bool LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(int trackID, std::chrono::steady_clock::time_point start, bool configDone)
{
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;

    // Try and make the program not crash... This usually is needed if disconnecting and reconnecting allot
    if (trackID == 4 && !configDone)
    {
        // printf("Time elapsed: %d", elapsed.count());

        if (elapsed.count() > 7000)
        {
            // Terminate thread
            LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadLR10K, &LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag);
            // Terminate thread (since we are disconnecting from COM port)
            LR10KDataDeEncryptionSystem::TerminateUniqeThread(LR10KDataDeEncryptionSystem::m_pThreadReadCom, &LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag);

            somethingUnexpectedHappend = true;
            printf("Connecting to LR 10K got stuck... Suggseting a restart of software and the LR 10K Tensile Machine\n");
        }
    }

    // Slow the refresh rate down (only for debugging)
    /*using namespace std::chrono_literals;
    std::this_thread::sleep_for(80ms);*/

    // printf("ThreadCallTracker: %d\n", trackID);
    return !LR10KDataDeEncryptionSystem::m_pThreadReadCom_stopFlag;
}
/**
    @brief  If threads hooks are allowed to continue running.
    @retval  -
**/
bool LR10KDataDeEncryptionSystem::ThreadCanRun_InterpetWrittenRingBufferData()
{
    // Slow the refresh rate down (only for debugging)
    /*using namespace std::chrono_literals;
    std::this_thread::sleep_for(20ms);*/

    return !LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag;
}
/**
    @brief (Step End) Stop/terminate a unique thread that is running a hook.
    @param stopFlag  -
    @param m_pThread -
**/
void LR10KDataDeEncryptionSystem::TerminateUniqeThread(std::unique_ptr<std::thread>& m_pThread, std::atomic<bool>* stopFlag)
{
    if (stopFlag != nullptr)
        *stopFlag = true;

    // When the std::unique_ptr is destroyed, it will automatically release ownership of the thread object and delete the thread (i.e., call std::thread::join()) if it is joinable. 
    if (m_pThread && m_pThread->joinable()) {
        // Merge thread with main to "destroy" it
        m_pThread->join();

        // Reset
        m_pThread = nullptr;

        // Allow time for the detached threads to exit
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
