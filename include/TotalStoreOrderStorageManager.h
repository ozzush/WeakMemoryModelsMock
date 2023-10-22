//
// Created by veronika on 21.10.23.
//

#pragma once

#include <algorithm>
#include <deque>
#include <optional>
#include <random>

#include "Storage.h"
#include "StorageManager.h"

namespace wmm {

struct StoreInstruction {
    const size_t address;
    const int32_t value;
};

class Buffer {
    std::deque<StoreInstruction> m_buffer;

public:
    void push(StoreInstruction instruction);
    std::optional<StoreInstruction> pop();
    std::optional<int32_t> find(size_t address);
    friend std::ostream &operator<<(std::ostream &os, const Buffer &buffer);
};

class InternalUpdateManager;
class SequentialTSOInternalUpdateManager;
class RandomTSOInternalUpdateManager;

using InternalUpdateManagerPtr = std::unique_ptr<InternalUpdateManager>;

class TotalStoreOrderStorageManager : public StorageManagerInterface {
    Storage m_storage;
    std::vector<Buffer> m_threadBuffers;
    InternalUpdateManagerPtr m_internalUpdateManager;

    void flushBuffer(size_t threadId);
    bool propagate(size_t threadId);

public:
    TotalStoreOrderStorageManager(
            size_t storageSize, size_t nOfThreads,
            InternalUpdateManagerPtr &&internalUpdateManager)
        : m_storage(storageSize), m_threadBuffers(nOfThreads),
          m_internalUpdateManager(std::move(internalUpdateManager)) {}

    int32_t load(size_t threadId, size_t address,
                 MemoryAccessMode accessMode) override;
    void store(size_t threadId, size_t address, int32_t value,
               MemoryAccessMode accessMode) override;
    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue,
                        int32_t newValue, MemoryAccessMode accessMode) override;
    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment,
                           MemoryAccessMode accessMode) override;
    void fence(size_t threadId, MemoryAccessMode accessMode) override;

    [[nodiscard]] Storage getStorage() const override { return m_storage; }
    void writeStorage(std::ostream &outputStream) const override;
    bool internalUpdate() override;

    friend class SequentialTSOInternalUpdateManager;
    friend class RandomTSOInternalUpdateManager;
};

class InternalUpdateManager {
    virtual void reset(const TotalStoreOrderStorageManager &storageManager) = 0;
    virtual std::optional<size_t> getThreadId() = 0;

    friend class TotalStoreOrderStorageManager;

public:
    virtual ~InternalUpdateManager() = default;
};

class SequentialTSOInternalUpdateManager : public InternalUpdateManager {
    size_t m_nextThreadId = 0;
    size_t m_maxThreadId = 0;

    void reset(const TotalStoreOrderStorageManager &storageManager) override;
    std::optional<size_t> getThreadId() override;

public:
    SequentialTSOInternalUpdateManager() = default;
};

class RandomTSOInternalUpdateManager : public InternalUpdateManager {
    std::vector<size_t> m_threadIds;
    size_t m_nextThreadIdIndex;
    std::mt19937 m_randomGenerator;

    void reset(const TotalStoreOrderStorageManager &storageManager) override;
    std::optional<size_t> getThreadId() override;

public:
    explicit RandomTSOInternalUpdateManager(unsigned long seed)
        : m_randomGenerator(seed), m_nextThreadIdIndex(0) {}
};

} // namespace wmm
