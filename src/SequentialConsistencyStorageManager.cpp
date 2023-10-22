//
// Created by veronika on 21.10.23.
//

#include "SequentialConsistencyStorageManager.h"

namespace wmm {

int32_t SequentialConsistencyStorageManager::load(size_t threadId,
                                                  size_t address,
                                                  MemoryAccessMode accessMode) {
    return m_storage.load(address);
}

void SequentialConsistencyStorageManager::store(size_t threadId, size_t address,
                                                int32_t value,
                                                MemoryAccessMode accessMode) {
    m_storage.store(address, value);
}

void SequentialConsistencyStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    auto value = m_storage.load(address);
    if (value == expectedValue) { m_storage.store(address, newValue); }
}

void SequentialConsistencyStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    auto value = m_storage.load(address);
    m_storage.store(address, value + increment);
}

void SequentialConsistencyStorageManager::fence(size_t threadId,
                                                MemoryAccessMode accessMode) {}
void SequentialConsistencyStorageManager::writeStorage(
        std::ostream &outputStream) const {
    outputStream << "Shared storage: " << m_storage << '\n';
}


} // namespace wmm