//
// Created by veronika on 21.10.23.
//

#include <ostream>

#include "TotalStoreOrderStorageManager.h"

namespace wmm::storage {

int32_t TotalStoreOrderStorageManager::load(size_t threadId, size_t address,
                                            MemoryAccessMode accessMode) {
    auto valueFromBuffer = m_threadBuffers.at(threadId).find(address);
    if (valueFromBuffer) { return valueFromBuffer.value(); }
    return m_storage.load(address);
}

void TotalStoreOrderStorageManager::store(size_t threadId, size_t address,
                                          int32_t value,
                                          MemoryAccessMode accessMode) {
    m_threadBuffers.at(threadId).push({address, value});
}

void TotalStoreOrderStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    auto value = m_storage.load(address);
    if (value == expectedValue) { m_storage.store(address, newValue); }
}

void TotalStoreOrderStorageManager::flushBuffer(size_t threadId) {
    while (propagate(threadId)) {}
}

void TotalStoreOrderStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    auto value = m_storage.load(address);
    m_storage.store(address, value + increment);
}

void TotalStoreOrderStorageManager::fence(size_t threadId,
                                          MemoryAccessMode accessMode) {
    flushBuffer(threadId);
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
        os << instruction.address << "->" << instruction.value;
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