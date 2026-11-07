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

// The decoding itself is in LR10KProtocol.h; LR10KMachine applies it to every message (LR10KReadings).

/**
    @brief (Step 2) Handle one complete message from the LR 10K (runs on the interpreter thread, see LR10KMachine).
            The machine has already decoded it into @p readings; this writes the values to the globals the GUI uses.
    @param message  - Framed by LR10KProtocol::MessageParser.
    @param readings - The machine's readings after this message.
**/
void LR10KDataDeEncryptionSystem::InterpretMessage(const LR10KProtocol::Message& message, const LR10KReadings& readings)
{
    using Type = LR10KProtocol::Message::Type;

    switch (message.type)
    {
    case Type::DataPackage:
    {
        // Load, distance and extensometer. These need to be multiplied by a constant in namespace
        // "LR10KAppGlobals" for readability.
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)readings.load, 1);
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)readings.distanceDelta, 2);
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)readings.extensometer, 3);
        break;
    }
    case Type::Overflow:
        // The multiplier for the load bytes
        // The machine uses it for the following data packages (LR10KReadings::loadMultiplier)
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)readings.loadMultiplier, 4);
        break;
    case Type::Status:
        // The actual status (ready, up, down, fast up etc.)
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)message.Value(), 5);
        break;
    case Type::WarnError:
        // Warning/error status (e.g. magnetic stop)
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)message.Value(), 6);
        break;
    default:
        // Unknown (as of now unknown data)
        InterpetWrittenRingBufferData_WriteGlobals((LPARAM)message.data[0]);
        break;
    }
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
            LR10KDataDeEncryptionSystem::m_pThreadReadLR10K_stopFlag.store(true);
            LR10KDataDeEncryptionSystem::machine.RequestStopInterpreter();
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
