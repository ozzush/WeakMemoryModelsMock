//
// Created by veronika on 21.10.23.
//

#include <format>
#include <sstream>

#include "SequentialConsistencyStorageManager.h"

namespace wmm::storage::SC {

int32_t SequentialConsistencyStorageManager::load(size_t threadId,
                                                  size_t address,
                                                  MemoryAccessMode accessMode) {
    int32_t result = m_storage.load(address);
    m_storageLogger->load(threadId, address, accessMode, result);
    return result;
}

void SequentialConsistencyStorageManager::store(size_t threadId, size_t address,
                                                int32_t value,
                                                MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    m_storage.store(address, value);
}

void SequentialConsistencyStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    auto value = m_storage.load(address);
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, value,
                                    newValue, accessMode);
    if (value == expectedValue) {
        m_storage.store(address, newValue);
    }
}

void SequentialConsistencyStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    m_storageLogger->fetchAndIncrement(threadId, address, increment,
                                       accessMode);
    auto value = m_storage.load(address);
    m_storage.store(address, value + increment);
}

void SequentialConsistencyStorageManager::fence(size_t threadId,
                                                MemoryAccessMode accessMode) {
    m_storageLogger->fence(threadId, accessMode);
}

void SequentialConsistencyStorageManager::writeStorage(
        std::ostream &outputStream) const {
    outputStream << "Shared storage: " << m_storage.str() << '\n';
}


} // namespace wmm::storage