//
// Created by veronika on 21.10.23.
//

#pragma once

#include <format>
#include <memory>

#include "Instructions.h"
#include "Logger.h"
#include "Storage.h"

namespace wmm::storage {

enum class MemoryAccessMode {
    SequentialConsistency = 0,
    Release,
    Acquire,
    ReleaseAcquire,
    Relaxed
};

inline std::string toString(MemoryAccessMode memoryAccessMode) {
    switch (memoryAccessMode) {
        case MemoryAccessMode::SequentialConsistency:
            return "SEQ_CST";
        case MemoryAccessMode::Release:
            return "REL";
        case MemoryAccessMode::Acquire:
            return "ACQ";
        case MemoryAccessMode::ReleaseAcquire:
            return "REL_ACQ";
        case MemoryAccessMode::Relaxed:
            return "RLX";
    }
}

class StorageManagerInterface {
protected:
    LoggerPtr m_storageLogger;

    explicit StorageManagerInterface(LoggerPtr storageLogger)
        : m_storageLogger(std::move(storageLogger)) {}

public:
    virtual int32_t load(size_t threadId, size_t address,
                         MemoryAccessMode accessMode) {
        m_storageLogger->info(std::format("t{} at #{}: {} load", threadId,
                                          address, toString(accessMode)));
        return 0;
    };
    virtual void store(size_t threadId, size_t address, int32_t value,
                       MemoryAccessMode accessMode) {
        m_storageLogger->info(std::format("t{} at #{}: {} store {}", threadId,
                                          address, toString(accessMode),
                                          value));
    }
    virtual void compareAndSwap(size_t threadId, size_t address,
                                int32_t expectedValue, int32_t newValue,
                                MemoryAccessMode accessMode) {
        m_storageLogger->info(
                std::format("t{} at #{}: {} if {} store {}", threadId, address,
                            toString(accessMode), expectedValue, newValue));
    }
    virtual void fetchAndIncrement(size_t threadId, size_t address,
                                   int32_t increment,
                                   MemoryAccessMode accessMode) {
        m_storageLogger->info(std::format("t{} at #{}: {} +{}", threadId,
                                          address, toString(accessMode),
                                          increment));
    }
    virtual void fence(size_t threadId, MemoryAccessMode accessMode) {
        m_storageLogger->info(
                std::format("t{}: {} fence", threadId, toString(accessMode)));
    }

    virtual bool internalUpdate() { return false; };
    [[nodiscard]] virtual Storage getStorage() const = 0;

    virtual void writeStorage(std::ostream &outputStream) const = 0;

    virtual void logStorage() const {
        m_storageLogger->storage(*this);
    }
};

using StorageManagerPtr = std::shared_ptr<StorageManagerInterface>;

class FakeStorageManager : public StorageManagerInterface {
public:
    int32_t load(size_t threadId, size_t address,
                 MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void store(size_t threadId, size_t address, int32_t value,
               MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue,
                        int32_t newValue,
                        MemoryAccessMode accessMode) override {
        throw std::logic_error("Not implemented");
    }
    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment,
                           MemoryAccessMode accessMode) override {
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

} // namespace wmm::storage
