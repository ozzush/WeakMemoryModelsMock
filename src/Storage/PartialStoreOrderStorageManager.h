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

namespace wmm::storage::PSO {

class AddressBuffer {
    std::deque<int32_t> m_buffer;

public:
    void push(int32_t value);
    [[nodiscard]] std::optional<int32_t> last() const;
    std::optional<int32_t> pop();
    [[nodiscard]] bool empty() const;

    [[nodiscard]] std::string str() const;
};

class ThreadBuffer {
    std::vector<AddressBuffer> m_buffer;

public:
    void push(size_t address, int32_t value);
    std::optional<int32_t> pop(size_t address);
    std::optional<int32_t> find(size_t address);
    [[nodiscard]] const AddressBuffer &getBuffer(size_t address) const;
    [[nodiscard]] std::string str(size_t address) const;
    [[nodiscard]] std::string str() const;

    explicit ThreadBuffer(size_t size) : m_buffer(size) {}
};

class InternalUpdateManager;
class SequentialInternalUpdateManager;
class RandomInternalUpdateManager;

using InternalUpdateManagerPtr = std::unique_ptr<InternalUpdateManager>;

class PartialStoreOrderStorageManager : public StorageManagerInterface {
    Storage m_storage;
    std::vector<ThreadBuffer> m_threadBuffers;
    InternalUpdateManagerPtr m_internalUpdateManager;

    void flushBuffer(size_t threadId, size_t address);
    bool propagate(size_t threadId, size_t address);

public:
    PartialStoreOrderStorageManager(
            size_t storageSize, size_t nOfThreads,
            InternalUpdateManagerPtr &&internalUpdateManager,
            LoggerPtr &&logger = std::make_unique<FakeStorageLogger>())
        : StorageManagerInterface(std::move(logger)), m_storage(storageSize),
          m_threadBuffers(nOfThreads, ThreadBuffer(storageSize)),
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

    void writeStorage(std::ostream &outputStream) const override;
    bool internalUpdate() override;

    friend class SequentialInternalUpdateManager;
    friend class RandomInternalUpdateManager;
    friend class InteractiveInternalUpdateManager;
};

class InternalUpdateManager {
    virtual void
    reset(const PartialStoreOrderStorageManager &storageManager) = 0;

    virtual std::optional<std::pair<size_t, size_t>>
    getThreadIdAndAddress() = 0;

    friend class PartialStoreOrderStorageManager;

public:
    virtual ~InternalUpdateManager() = default;
};

class SequentialInternalUpdateManager : public InternalUpdateManager {
    size_t m_nextThreadId = 0;
    size_t m_maxThreadId = 0;
    size_t m_nextAddress = 0;
    size_t m_maxAddress = 0;

    void reset(const PartialStoreOrderStorageManager &storageManager) override;
    std::optional<std::pair<size_t, size_t>> getThreadIdAndAddress() override;

public:
    SequentialInternalUpdateManager() = default;
};

class RandomInternalUpdateManager : public InternalUpdateManager {
    std::vector<std::pair<size_t, size_t>> m_threadIdAndAddressPairs;
    size_t m_nextThreadIdAndAddressIndex;
    std::mt19937 m_randomGenerator;

    void reset(const PartialStoreOrderStorageManager &storageManager) override;
    std::optional<std::pair<size_t, size_t>> getThreadIdAndAddress() override;

public:
    explicit RandomInternalUpdateManager(unsigned long seed)
        : m_randomGenerator(seed), m_nextThreadIdAndAddressIndex(0) {}
};

class InteractiveInternalUpdateManager : public InternalUpdateManager {
    std::vector<std::pair<size_t, size_t>> m_threadIdAndAddressPairs;
    std::vector<std::reference_wrapper<const AddressBuffer>> m_buffers;

    void reset(const PartialStoreOrderStorageManager &storageManager) override;
    std::optional<std::pair<size_t, size_t>> getThreadIdAndAddress() override;

public:
    InteractiveInternalUpdateManager() = default;
};

} // namespace wmm::storage::PSO
