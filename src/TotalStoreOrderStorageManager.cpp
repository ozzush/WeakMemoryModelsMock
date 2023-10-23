//
// Created by veronika on 21.10.23.
//

#include <format>
#include <ostream>
#include <sstream>

#include "TotalStoreOrderStorageManager.h"

namespace wmm::storage {

int32_t TotalStoreOrderStorageManager::load(size_t threadId, size_t address,
                                            MemoryAccessMode accessMode) {
    m_storageLogger->load(threadId, address, accessMode);
    auto valueFromBuffer = m_threadBuffers.at(threadId).find(address);
    if (valueFromBuffer) { return valueFromBuffer.value(); }
    return m_storage.load(address);
}

void TotalStoreOrderStorageManager::store(size_t threadId, size_t address,
                                          int32_t value,
                                          MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    m_threadBuffers.at(threadId).push({address, value});
    logBuffer(threadId);
}

void TotalStoreOrderStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, newValue,
                                    accessMode);
    auto value = m_storage.load(address);
    if (value == expectedValue) {
        m_storage.store(address, newValue);
        logStorage();
    }
}

void TotalStoreOrderStorageManager::flushBuffer(size_t threadId) {
    while (propagate(threadId)) {}
}

void TotalStoreOrderStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    m_storageLogger->fetchAndIncrement(threadId, address, increment,
                                       accessMode);
    auto value = m_storage.load(address);
    m_storage.store(address, value + increment);
    logStorage();
}

void TotalStoreOrderStorageManager::fence(size_t threadId,
                                          MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    m_storageLogger->fence(threadId, accessMode);
}

void TotalStoreOrderStorageManager::writeStorage(
        std::ostream &outputStream) const {
    outputStream << "Shared storage: " << m_storage << '\n';
    outputStream << "Thread buffers:\n";
    for (size_t i = 0; i < m_threadBuffers.size(); ++i) {
        outputStream << 't' << i << ": " << m_threadBuffers[i] << '\n';
    }
}

bool TotalStoreOrderStorageManager::propagate(size_t threadId) {
    auto instruction = m_threadBuffers.at(threadId).pop();
    if (instruction) {
        m_storage.store(instruction->address, instruction->value);
        m_storageLogger->info(std::format("ACTION: b{}: #{}->{}", threadId,
                                          instruction->address,
                                          instruction->value));
        logBuffer(threadId);
        return true;
    }
    return false;
}

bool TotalStoreOrderStorageManager::internalUpdate() {
    m_internalUpdateManager->reset(*this);
    while (auto threadId = m_internalUpdateManager->getThreadId()) {
        if (propagate(threadId.value())) { return true; }
    }
    return false;
}

void TotalStoreOrderStorageManager::logBuffer(size_t threadId) {
    std::stringstream bufferStream;
    bufferStream << m_threadBuffers.at(threadId);
    m_storageLogger->info(
            std::format("STATE:  b{}: {}", threadId, bufferStream.str()));
}

void TotalStoreOrderStorageManager::logStorage() {
    std::stringstream storageStream;
    storageStream << m_storage;
    m_storageLogger->info(std::format("STATE:  storage: {}", storageStream.str()));
}

std::optional<StoreInstruction> Buffer::pop() {
    if (m_buffer.empty()) { return {}; }
    auto returnValue = m_buffer.front();
    m_buffer.pop_front();
    return returnValue;
}

std::optional<int32_t> Buffer::find(size_t address) {
    auto elm = std::find_if(m_buffer.rbegin(), m_buffer.rend(),
                            [address](auto instruction) {
                                return instruction.address == address;
                            });
    if (elm == m_buffer.rend()) { return {}; }
    return elm->value;
}

std::ostream &operator<<(std::ostream &os, const Buffer &buffer) {
    bool isFirstIteration = true;
    for (auto instruction: buffer.m_buffer) {
        if (!isFirstIteration) os << ' ';
        os << std::format("#{}->{}", instruction.address, instruction.value);
        isFirstIteration = false;
    }
    return os;
}

void Buffer::push(StoreInstruction instruction) {
    m_buffer.push_back(instruction);
};

void SequentialTSOInternalUpdateManager::reset(
        const TotalStoreOrderStorageManager &storageManager) {
    m_nextThreadId = 0;
    m_maxThreadId = storageManager.m_threadBuffers.size();
}

std::optional<size_t> SequentialTSOInternalUpdateManager::getThreadId() {
    if (m_nextThreadId < m_maxThreadId) { return m_nextThreadId++; }
    return {};
}

void RandomTSOInternalUpdateManager::reset(
        const TotalStoreOrderStorageManager &storageManager) {
    m_threadIds.resize(storageManager.m_threadBuffers.size());
    for (int i = 0; i < m_threadIds.size(); ++i) { m_threadIds[i] = i; }
    std::shuffle(m_threadIds.begin(), m_threadIds.end(), m_randomGenerator);
    m_nextThreadIdIndex = 0;
}

std::optional<size_t> RandomTSOInternalUpdateManager::getThreadId() {
    if (m_nextThreadIdIndex < m_threadIds.size()) {
        return m_threadIds[m_nextThreadIdIndex++];
    }
    return {};
}

} // namespace wmm::storage