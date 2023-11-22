//
// Created by veronika on 21.10.23.
//

#pragma once

#include "Storage.h"
#include "StorageManager.h"

namespace wmm::storage::SC {

class SequentialConsistencyStorageManager : public StorageManagerInterface {
    Storage m_storage;

public:
    explicit SequentialConsistencyStorageManager(
            size_t storageSize,
            storage::LoggerPtr &&logger = std::make_unique<FakeStorageLogger>())
        : StorageManagerInterface(std::move(logger)), m_storage(storageSize) {}

    int32_t load(size_t threadId, size_t address,
                 MemoryAccessMode accessMode) override;
    void store(size_t threadId, size_t address, int32_t value,
               MemoryAccessMode accessMode) override;
    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue,
                        int32_t newValue, MemoryAccessMode accessMode) override;
    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment,
                           MemoryAccessMode accessMode) override;
    void fence(size_t threadId, MemoryAccessMode accessMode) override;

    void writeStorage(std::ostream &outputStream) const override;
};

} // namespace wmm::storage
