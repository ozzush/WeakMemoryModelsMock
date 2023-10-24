//
// Created by veronika on 21.10.23.
//

#include <format>
#include <iostream>
#include <ostream>
#include <sstream>

#include "TotalStoreOrderStorageManager.h"

namespace wmm::storage::TSO {

int32_t TotalStoreOrderStorageManager::load(size_t threadId, size_t address,
                                            MemoryAccessMode accessMode) {
    auto valueFromBuffer = m_threadBuffers.at(threadId).find(address);
    int32_t value = (valueFromBuffer) ? valueFromBuffer.value()
                                      : m_storage.load(address);
    m_storageLogger->load(threadId, address, accessMode, value);
    return value;
}

void TotalStoreOrderStorageManager::store(size_t threadId, size_t address,
                                          int32_t value,
                                          MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    m_threadBuffers.at(threadId).push({address, value});
}

void TotalStoreOrderStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    auto value = m_storage.load(address);
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, value,
                                    newValue, accessMode);
    if (value == expectedValue) { m_storage.store(address, newValue); }
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
}

void TotalStoreOrderStorageManager::fence(size_t threadId,
                                          MemoryAccessMode accessMode) {
    flushBuffer(threadId);
    m_storageLogger->fence(threadId, accessMode);
}

void TotalStoreOrderStorageManager::writeStorage(
        std::ostream &outputStream) const {
    outputStream << "Shared storage: " << m_storage.str() << '\n';
    outputStream << "Thread buffers:\n";
    for (size_t i = 0; i < m_threadBuffers.size(); ++i) {
        outputStream << std::format("b{}: {}\n", i, m_threadBuffers[i].str());
    }
}

bool TotalStoreOrderStorageManager::propagate(size_t threadId) {
    auto instruction = m_threadBuffers.at(threadId).pop();
    if (instruction) {
        m_storage.store(instruction->address, instruction->value);
        m_storageLogger->info(std::format("ACTION: b{}: propagate ({})",
                                          threadId, instruction->str()));
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

void Buffer::push(StoreInstruction instruction) {
    m_buffer.push_back(instruction);
}

bool Buffer::empty() const { return m_buffer.empty(); }

std::string Buffer::str() const {
    std::string result;
    bool isFirstIteration = true;
    for (auto instruction: m_buffer) {
        if (!isFirstIteration) result += ' ';
        result += instruction.str();
        isFirstIteration = false;
    }
    return result;
};

void SequentialInternalUpdateManager::reset(
        const TotalStoreOrderStorageManager &storageManager) {
    m_nextThreadId = 0;
    m_maxThreadId = storageManager.m_threadBuffers.size();
}

std::optional<size_t> SequentialInternalUpdateManager::getThreadId() {
    if (m_nextThreadId < m_maxThreadId) { return m_nextThreadId++; }
    return {};
}

void RandomInternalUpdateManager::reset(
        const TotalStoreOrderStorageManager &storageManager) {
    m_threadIds.resize(storageManager.m_threadBuffers.size());
    for (int i = 0; i < m_threadIds.size(); ++i) { m_threadIds[i] = i; }
    std::shuffle(m_threadIds.begin(), m_threadIds.end(), m_randomGenerator);
    m_nextThreadIdIndex = 0;
}

std::optional<size_t> RandomInternalUpdateManager::getThreadId() {
    if (m_nextThreadIdIndex < m_threadIds.size()) {
        return m_threadIds[m_nextThreadIdIndex++];
    }
    return {};
}

std::optional<size_t> InteractiveInternalUpdateManager::getThreadId() {
    if (m_threadIds.empty()) {
        std::cout << "All buffers are empty.\n";
        return {};
    }
    std::cout << "Choose buffer to propagate:\n";
    for (size_t i = 0; i < m_threadIds.size(); ++i) {
        std::cout << std::format("{}: {}\n", m_threadIds[i],
                                 m_buffers[i].get().str());
    }
    while (true) {
        size_t threadId;
        std::cout << "Enter buffer id > ";
        std::cin >> threadId;
        if (std::cin.eof() || std::cin.fail()) { return {}; }
        if (std::count(m_threadIds.begin(), m_threadIds.end(), threadId) == 0) {
            std::cout << "This buffer cannot be propagated.\n";
        } else {
            return threadId;
        }
    }
}

void InteractiveInternalUpdateManager::reset(
        const TotalStoreOrderStorageManager &storageManager) {
    m_threadIds.clear();
    m_buffers.clear();
    for (size_t i = 0; i < storageManager.m_threadBuffers.size(); ++i) {
        if (!storageManager.m_threadBuffers[i].empty()) {
            m_threadIds.push_back(i);
            m_buffers.emplace_back(storageManager.m_threadBuffers.at(i));
        }
    }
}

std::string StoreInstruction::str() const {
    return std::format("#{}->{}", address, value);
}
} // namespace wmm::storage::TSO