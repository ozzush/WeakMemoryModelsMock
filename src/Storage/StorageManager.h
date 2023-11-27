//
// Created by veronika on 21.10.23.
//

#pragma once

#include <memory>

#include "Instructions.h"
#include "Storage.h"
#include "StorageLogger.h"
#include "StorageMemoryAccessMode.h"

namespace wmm::storage {

class StorageManagerInterface {
protected:
    LoggerPtr m_storageLogger;

    explicit StorageManagerInterface(LoggerPtr storageLogger = nullptr)
        : m_storageLogger(std::move(storageLogger)) {}

public:
    virtual int32_t load(size_t threadId, size_t address,
                         MemoryAccessMode accessMode) = 0;

    virtual void store(size_t threadId, size_t address, int32_t value,
                       MemoryAccessMode accessMode) = 0;

    virtual void compareAndSwap(size_t threadId, size_t address,
                                int32_t expectedValue, int32_t newValue,
                                MemoryAccessMode accessMode) = 0;

    virtual void fetchAndIncrement(size_t threadId, size_t address,
                                   int32_t increment,
                                   MemoryAccessMode accessMode) = 0;

    virtual void fence(size_t threadId, MemoryAccessMode accessMode) = 0;

    /**
     * Perform some internal updates
     *
     * @return false if no internal updates were performed, true otherwise
     */
    virtual bool internalUpdate() { return false; };

    virtual void writeStorage(std::ostream &outputStream) const = 0;
};

using StorageManagerPtr = std::shared_ptr<StorageManagerInterface>;

} // namespace wmm::storage
