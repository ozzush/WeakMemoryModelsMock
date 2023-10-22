//
// Created by veronika on 21.10.23.
//

#pragma once

#include "Instructions.h"
#include "Storage.h"
#include <memory>
namespace wmm {

class StorageManagerInterface {
public:
    virtual int32_t load(size_t threadId, size_t address, MemoryAccessMode accessMode) = 0;
    virtual void store(size_t threadId, size_t address, int32_t value, MemoryAccessMode accessMode) = 0;
    virtual void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue, int32_t newValue, MemoryAccessMode accessMode) = 0;
    virtual void fetchAndIncrement(size_t threadId, size_t address, int32_t increment, MemoryAccessMode accessMode) = 0;
    virtual void fence(size_t threadId, MemoryAccessMode accessMode) = 0;
    virtual bool internalUpdate() { return false; };

    [[nodiscard]] virtual Storage getStorage() const = 0;
    virtual void writeStorage(std::ostream &outputStream) const = 0;
};

using StorageManagerPtr = std::shared_ptr<StorageManagerInterface>;

class FakeStorageManager : public StorageManagerInterface {
public:
    int32_t load(size_t threadId, size_t address, MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void store(size_t threadId, size_t address, int32_t value, MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue, int32_t newValue, MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment, MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void fence(size_t threadId, MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }

    [[nodiscard]] Storage getStorage() const override {
        throw std::logic_error("Not implemented");
    }

    void writeStorage(std::ostream &outputStream) const override {}

    FakeStorageManager() = default;
};

} // namespace wmm
