//
// Created by veronika on 21.10.23.
//

#pragma once

#include "Storage.h"
#include "StorageManager.h"
#include <algorithm>
#include <deque>
#include <optional>
#include <random>
namespace wmm {

struct StoreInstruction {
    const size_t address;
    const int32_t value;
};

class Buffer {
    std::deque<StoreInstruction> m_buffer;

public:
    void push(StoreInstruction instruction) {
        m_buffer.push_back(instruction);
    };

    std::optional<StoreInstruction> pop() {
        if (m_buffer.empty()) { return {}; }
        auto returnValue = m_buffer.front();
        m_buffer.pop_front();
        return returnValue;
    }

    std::optional<int32_t> find(size_t address) {
        auto elm = std::find_if(m_buffer.rbegin(), m_buffer.rend(),
                                [address](auto instruction) {
                                    return instruction.address == address;
                                });
        if (elm == m_buffer.rend()) { return {}; }
        return elm->value;
    }

    friend std::ostream &operator<<(std::ostream &os, const Buffer &buffer) {
        bool isFirstIteration = true;
        for (auto instruction: buffer.m_buffer) {
            if (!isFirstIteration) os << ' ';
            os << instruction.address << "->" << instruction.value;
            isFirstIteration = false;
        }
        return os;
    }
};

class TotalStoreOrderStorageManager;

class InternalUpdateManager {
    virtual void reset(const TotalStoreOrderStorageManager &storageManager) = 0;
    virtual std::optional<size_t> getThreadId() = 0;

    friend class TotalStoreOrderStorageManager;

public:
    virtual ~InternalUpdateManager() = default;
};

using InternalUpdateManagerPtr = std::unique_ptr<InternalUpdateManager>;

class SequentialTSOInternalUpdateManager;
class RandomTSOInternalUpdateManager;

class TotalStoreOrderStorageManager : public StorageManagerInterface {
    Storage m_storage;
    std::vector<Buffer> m_threadBuffers;
    InternalUpdateManagerPtr m_internalUpdateManager;

    void flushBuffer(size_t threadId) {
        while (propagate(threadId))
            ;
    }

    bool propagate(size_t threadId) {
        auto instruction = m_threadBuffers.at(threadId).pop();
        if (instruction) {
            m_storage.store(instruction->address, instruction->value);
            return true;
        }
        return false;
    }

public:
    TotalStoreOrderStorageManager(
            size_t storageSize, size_t nOfThreads,
            InternalUpdateManagerPtr &&internalUpdateManager)
        : m_storage(storageSize), m_threadBuffers(nOfThreads),
          m_internalUpdateManager(std::move(internalUpdateManager)) {}

    int32_t load(size_t threadId, size_t address,
                 MemoryAccessMode accessMode) override {
        auto valueFromBuffer = m_threadBuffers.at(threadId).find(address);
        if (valueFromBuffer) { return valueFromBuffer.value(); }
        return m_storage.load(address);
    }

    void store(size_t threadId, size_t address, int32_t value,
               MemoryAccessMode accessMode) override {
        m_threadBuffers.at(threadId).push({address, value});
    }

    void compareAndSwap(size_t threadId, size_t address, int32_t expectedValue,
                        int32_t newValue,
                        MemoryAccessMode accessMode) override {
        flushBuffer(threadId);
        auto value = m_storage.load(address);
        if (value == expectedValue) { m_storage.store(address, newValue); }
    }

    void fetchAndIncrement(size_t threadId, size_t address, int32_t increment,
                           MemoryAccessMode accessMode) override {
        flushBuffer(threadId);
        auto value = m_storage.load(address);
        m_storage.store(address, value + increment);
    }

    void fence(size_t threadId, MemoryAccessMode accessMode) override {
        flushBuffer(threadId);
    }

    [[nodiscard]] Storage getStorage() const override { return m_storage; }

    void writeStorage(std::ostream &outputStream) const override {
        outputStream << "Shared storage: " << m_storage << '\n';
        outputStream << "Thread buffers:\n";
        for (size_t i = 0; i < m_threadBuffers.size(); ++i) {
            outputStream << 't' << i << ": " << m_threadBuffers[i] << '\n';
        }
    }

    bool internalUpdate() override {
        m_internalUpdateManager->reset(*this);
        while (auto threadId = m_internalUpdateManager->getThreadId()) {
            if (propagate(threadId.value())) { return true; }
        }
        return false;
    }

    friend class SequentialTSOInternalUpdateManager;
    friend class RandomTSOInternalUpdateManager;
};

class SequentialTSOInternalUpdateManager : public InternalUpdateManager {
    size_t m_nextThreadId = 0;
    size_t m_maxThreadId = 0;

    void reset(const TotalStoreOrderStorageManager &storageManager) override {
        m_nextThreadId = 0;
        m_maxThreadId = storageManager.m_threadBuffers.size();
    }

    std::optional<size_t> getThreadId() override {
        if (m_nextThreadId < m_maxThreadId) { return m_nextThreadId++; }
        return {};
    }

public:
    SequentialTSOInternalUpdateManager() = default;

};

class RandomTSOInternalUpdateManager : public InternalUpdateManager {
    std::vector<size_t> m_threadIds;
    size_t m_nextThreadIdIndex;
    std::mt19937 m_randomGenerator;

    void reset(const TotalStoreOrderStorageManager &storageManager) override {
        m_threadIds.resize(storageManager.m_threadBuffers.size());
        for (int i = 0; i < m_threadIds.size(); ++i) {
            m_threadIds[i] = i;
        }
        std::shuffle(m_threadIds.begin(), m_threadIds.end(), m_randomGenerator);
        m_nextThreadIdIndex = 0;
    }

    std::optional<size_t> getThreadId() override {
        if (m_nextThreadIdIndex < m_threadIds.size()) {
            return m_threadIds[m_nextThreadIdIndex++];
        }
        return {};
    }

public:
    explicit RandomTSOInternalUpdateManager(unsigned long seed) : m_randomGenerator(seed), m_nextThreadIdIndex(0) {}
};

} // namespace wmm
