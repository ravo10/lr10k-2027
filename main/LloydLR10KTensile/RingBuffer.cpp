/**

    @file      RingBuffer.cpp
    @brief     Contains logic for handling the write and read sequence (Serial Buffer) for the LR 10K Tensile Machine.
    @details   ~
    @author    Rasmus Ullestad Relling (orginal logic by Olaf Hallan Graven)
    @date      4.09.2023
    @copyright (c) Rasmus Ullestad Relling, 2023. All right reserved.

**/


#include "LR10KDecryption.h"
#include "iostream"

CRingBuffer::CRingBuffer(): m_uData(), m_uRead(0), m_uWrite(0), m_bEmpty(true) { }

bool CRingBuffer::Write(byte Element, bool NonBlocking /*= false*/)
{
    // We are full
    if (NonBlocking && !m_bEmpty && m_uRead == m_uWrite) return false;

    // Wait for space
    const auto start = std::chrono::high_resolution_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(3, start) && !m_bEmpty && m_uRead == m_uWrite);

    m_Protect.lock();
    m_uData[m_uWrite++] = Element;
    m_uWrite %= bufferSize;
    m_Protect.unlock();

    m_bEmpty = false;
    return true;
}

byte CRingBuffer::Read(bool configDone)
{
    byte ret; // = 0x00 when no data

    const auto start = std::chrono::high_resolution_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(4, start, configDone) && m_bEmpty);

    m_Protect.lock();
    ret = m_uData[m_uRead++];
    m_uRead %= bufferSize;
    m_Protect.unlock();

    if (m_uRead == m_uWrite)
        m_bEmpty = true;

    return ret;
}

bool CRingBuffer::DataAvailable()
{
    return !m_bEmpty;
}

void CRingBuffer::ReadOutAllData()
{
    byte ret; // = 0x00 when no data

    const auto start = std::chrono::high_resolution_clock::now();
    while (LR10KDataDeEncryptionSystem::ThreadCanRun_ReadFromLR10KAndWriteToRingBuffer(5, start) && m_bEmpty)
    {
        m_Protect.lock();
        ret = m_uData[m_uRead++];
        m_uRead %= bufferSize;
        m_Protect.unlock();

        if (m_uRead == m_uWrite)
            m_bEmpty = true;
    }
}

void CRingBuffer::ClearAllData()
{
    for (size_t i = 0; i < bufferSize; i++) m_uData[i] = NULL;
    m_uRead = 0;
    m_uWrite = 0;

    m_bEmpty = true;
}
