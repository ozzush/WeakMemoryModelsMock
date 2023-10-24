//
// Created by veronika on 21.10.23.
//

#include <format>
#include <iostream>
#include <ostream>

#include "PartialStoreOrderStorageManager.h"

namespace wmm::storage::PSO {

int32_t PartialStoreOrderStorageManager::load(size_t threadId, size_t address,
                                              MemoryAccessMode accessMode) {
    auto valueFromBuffer = m_threadBuffers.at(threadId).find(address);
    int32_t value = (valueFromBuffer) ? valueFromBuffer.value()
                                      : m_storage.load(address);
    m_storageLogger->load(threadId, address, accessMode, value);
    return value;
}

void PartialStoreOrderStorageManager::store(size_t threadId, size_t address,
                                            int32_t value,
                                            MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    m_threadBuffers.at(threadId).push(address, value);
}

void PartialStoreOrderStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    flushBuffer(threadId, address);
    auto value = m_storage.load(address);
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, value,
                                    newValue, accessMode);
    if (value == expectedValue) { m_storage.store(address, newValue); }
}

void PartialStoreOrderStorageManager::flushBuffer(size_t threadId,
                                                  size_t address) {
    while (propagate(threadId, address)) {}
}

void PartialStoreOrderStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    flushBuffer(threadId, address);
    m_storageLogger->fetchAndIncrement(threadId, address, increment,
                                       accessMode);
    auto value = m_storage.load(address);
    m_storage.store(address, value + increment);
}

void PartialStoreOrderStorageManager::fence(size_t threadId,
                                            MemoryAccessMode accessMode) {
    for (size_t address = 0; address < m_storage.size(); ++address) {
        flushBuffer(threadId, address);
    }
    m_storageLogger->fence(threadId, accessMode);
}

void PartialStoreOrderStorageManager::writeStorage(
        std::ostream &outputStream) const {
    outputStream << "Shared storage: " << m_storage.str() << '\n';
    outputStream << "Thread buffers:\n";
    for (size_t i = 0; i < m_threadBuffers.size(); ++i) {
        outputStream << std::format("b{}: {}\n", i, m_threadBuffers[i].str());
    }
}

bool PartialStoreOrderStorageManager::propagate(size_t threadId,
                                                size_t address) {
    auto newValue = m_threadBuffers.at(threadId).pop(address);
    if (newValue) {
        m_storage.store(address, newValue.value());
        m_storageLogger->info(std::format("ACTION: b{}#{}: propagate ({})",
                                          threadId, address, newValue.value()));
        return true;
    }
    return false;
}

bool PartialStoreOrderStorageManager::internalUpdate() {
    m_internalUpdateManager->reset(*this);
    while (auto threadIdAndAddress =
                   m_internalUpdateManager->getThreadIdAndAddress()) {
        auto [threadId, address] = threadIdAndAddress.value();
        if (propagate(threadId, address)) { return true; }
    }
    return false;
}

void ThreadBuffer::push(size_t address, int32_t value) {
    m_buffer.at(address).push(value);
}
std::optional<int32_t> ThreadBuffer::pop(size_t address) {
    return m_buffer.at(address).pop();
}

std::optional<int32_t> ThreadBuffer::find(size_t address) {
    return m_buffer.at(address).last();
}

std::string ThreadBuffer::str(size_t address) const {
    return std::format("#{}=[{}]", address, m_buffer.at(address).str());
}
std::string ThreadBuffer::str() const {
    std::string result;
    bool isFirstIteration = true;
    for (size_t address = 0; address < m_buffer.size(); ++address) {
        if (!isFirstIteration) result += ' ';
        result += str(address);
        isFirstIteration = false;
    }
    return result;
}

const AddressBuffer &ThreadBuffer::getBuffer(size_t address) const {
    return m_buffer.at(address);
}

void SequentialInternalUpdateManager::reset(
        const PartialStoreOrderStorageManager &storageManager) {
    m_nextThreadId = 0;
    m_maxThreadId = storageManager.m_threadBuffers.size();
    m_nextAddress = 0;
    m_maxAddress = storageManager.m_storage.size();
}

std::optional<std::pair<size_t, size_t>>
SequentialInternalUpdateManager::getThreadIdAndAddress() {
    if (m_nextAddress >= m_maxAddress) {
        ++m_nextThreadId;
        m_nextAddress = 0;
    }
    if (m_nextThreadId >= m_maxThreadId) { return {}; }
    return {{m_nextThreadId, m_nextAddress++}};
}

void RandomInternalUpdateManager::reset(
        const PartialStoreOrderStorageManager &storageManager) {
    m_threadIdAndAddressPairs.reserve(storageManager.m_threadBuffers.size() *
                                      storageManager.m_storage.size());
    for (size_t threadId = 0; threadId < storageManager.m_threadBuffers.size();
         ++threadId) {
        for (size_t address = 0; address < storageManager.m_storage.size();
             ++address) {
            m_threadIdAndAddressPairs.emplace_back(threadId, address);
        }
    }
    std::shuffle(m_threadIdAndAddressPairs.begin(),
                 m_threadIdAndAddressPairs.end(), m_randomGenerator);
    m_nextThreadIdAndAddressIndex = 0;
}

std::optional<std::pair<size_t, size_t>>
RandomInternalUpdateManager::getThreadIdAndAddress() {
    if (m_nextThreadIdAndAddressIndex < m_threadIdAndAddressPairs.size()) {
        return m_threadIdAndAddressPairs[m_nextThreadIdAndAddressIndex++];
    }
    return {};
}

void AddressBuffer::push(int32_t value) { m_buffer.push_back(value); }
std::optional<int32_t> AddressBuffer::pop() {
    if (m_buffer.empty()) { return {}; }
    int32_t value = m_buffer.front();
    m_buffer.pop_front();
    return value;
}

bool AddressBuffer::empty() const { return m_buffer.empty(); }

std::optional<int32_t> AddressBuffer::last() const {
    if (m_buffer.empty()) { return {}; }
    return m_buffer.back();
}

std::string AddressBuffer::str() const {
    std::string result;
    bool isFirstIteration = true;
    for (auto value: m_buffer) {
        if (!isFirstIteration) result += ' ';
        result += std::to_string(value);
        isFirstIteration = false;
    }
    return result;
}

void InteractiveInternalUpdateManager::reset(
        const PartialStoreOrderStorageManager &storageManager) {
    m_buffers.clear();
    m_threadIdAndAddressPairs.clear();
    for (size_t threadId = 0; threadId < storageManager.m_threadBuffers.size();
         ++threadId) {
        for (size_t address = 0; address < storageManager.m_storage.size();
             ++address) {
            const auto &buffer =
                    storageManager.m_threadBuffers[threadId].getBuffer(address);
            if (!buffer.empty()) {
                m_threadIdAndAddressPairs.emplace_back(threadId, address);
                m_buffers.emplace_back(buffer);
            }
        }
    }
}

std::optional<std::pair<size_t, size_t>>
InteractiveInternalUpdateManager::getThreadIdAndAddress() {
    if (m_threadIdAndAddressPairs.empty()) {
        std::cout << "All buffers are empty.\n";
        return {};
    }
    std::cout << "Choose buffer to propagate:\n";
    for (size_t i = 0; i < m_threadIdAndAddressPairs.size(); ++i) {
        auto pair = m_threadIdAndAddressPairs[i];
        std::cout << std::format("{}#{}: {}\n", pair.first, pair.second,
                                 m_buffers[i].get().str());
    }
    while (true) {
        size_t threadId;
        size_t address;
        std::cout << "Enter thread id and address (two integers separated by "
                     "space) > ";
        std::cin >> threadId >> address;
        if (std::cin.eof() || std::cin.fail()) { return {}; }
        if (std::count(m_threadIdAndAddressPairs.begin(),
                       m_threadIdAndAddressPairs.end(),
                       std::pair{threadId, address}) == 0) {
            std::cout << "This buffer cannot be propagated.\n";
        } else {
            return {{threadId, address}};
        }
    }
}

} // namespace wmm::storage::PSO