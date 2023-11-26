//
// Created by veronika on 14.11.23.
//

#pragma once

#include "StorageManager.h"
#include <deque>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_map>

namespace wmm::storage::SRA {

struct Message {
    const size_t location;
    const int32_t value;
    const size_t timestamp;

    Message(size_t location_, int32_t value_, size_t timestamp_)
        : location(location_), value(value_), timestamp(timestamp_) {}

    [[nodiscard]] std::string str() const;

    Message &operator=(Message &&) noexcept {
        throw std::runtime_error("Re-assignment is not allowed");
    }
    Message &operator=(const Message &) = delete;
    Message(Message &&) = default;
    Message(const Message &) = default;
};

class MessageBuffer {
private:
    std::deque<Message> m_buffer;

public:
    void push(Message message);
    void popN(size_t n);
    Message operator[](size_t i) const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] std::string str() const;
};

class ThreadState {
private:
    Storage m_localStorage;
    MessageBuffer m_outgoingBuffer;
    std::vector<size_t> m_bufferPositions;
    std::vector<size_t> m_locationTimestamps;

public:
    const size_t m_threadId;
    [[nodiscard]] std::optional<Message> readMessage(size_t i) const;
    bool write(size_t location, int32_t value, size_t timestamp);

    [[nodiscard]] int32_t read(size_t location) const {
        return m_localStorage.load(location);
    }

    bool write(Message message);

    ThreadState(size_t threadId, size_t nOfThreads, size_t storageSize)
        : m_threadId(threadId), m_localStorage(storageSize),
          m_bufferPositions(nOfThreads, 0), m_locationTimestamps(storageSize) {}

    size_t advanceBufferPos(size_t threadId);

    void popNFromBuffer(size_t n) { m_outgoingBuffer.popN(n); }

    void resetBufferPosByN(size_t threadId, size_t n) {
        m_bufferPositions[threadId] -= n;
    }

    [[nodiscard]] size_t getBufferPos(size_t threadId) const;

    [[nodiscard]] std::string str() const;

    [[nodiscard]] size_t timestamp(size_t location) const {
        return m_locationTimestamps[location];
    }
};

class InternalUpdateManager;
class RandomInternalUpdateManager;
class InteractiveInternalUpdateManager;

using InternalUpdateManagerPtr = std::unique_ptr<InternalUpdateManager>;

class StrongReleaseAcquireStorageManager : public StorageManagerInterface {
private:
    std::vector<ThreadState> m_threads;
    InternalUpdateManagerPtr m_internalUpdateManager;
    std::vector<size_t> m_locationTimestamps;

    bool writeFromBuffer(size_t threadId, size_t otherThreadId);
    void write(size_t threadId, size_t location, int32_t value);
    void cleanUpBuffer(size_t threadId);
    [[nodiscard]] size_t minBufferPos(size_t threadId) const;

public:
    explicit StrongReleaseAcquireStorageManager(
            size_t storageSize, size_t nOfThreads,
            InternalUpdateManagerPtr &&internalUpdateManager,
            storage::LoggerPtr &&logger = std::make_unique<FakeStorageLogger>())
        : StorageManagerInterface(std::move(logger)),
          m_locationTimestamps(storageSize),
          m_internalUpdateManager(std::move(internalUpdateManager)) {
        m_threads.reserve(nOfThreads);
        for (size_t threadId = 0; threadId < nOfThreads; ++threadId) {
            m_threads.emplace_back(threadId, nOfThreads, storageSize);
        }
    }

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

    friend class RandomInternalUpdateManager;
    friend class InteractiveInternalUpdateManager;
};

class InternalUpdateManager {
    virtual void
    reset(const StrongReleaseAcquireStorageManager &storageManager) = 0;

    virtual std::optional<std::pair<size_t, size_t>> getThreadIds() = 0;

    friend class StrongReleaseAcquireStorageManager;

public:
    virtual ~InternalUpdateManager() = default;
};

class RandomInternalUpdateManager : public InternalUpdateManager {
    std::vector<size_t> m_threadIds;
    std::vector<size_t> m_otherThreadIds;
    size_t m_nextThreadIdIndex;
    size_t m_nextOtherThreadIdIndex;
    std::mt19937 m_randomGenerator;

    void
    reset(const StrongReleaseAcquireStorageManager &storageManager) override;
    std::optional<std::pair<size_t, size_t>> getThreadIds() override;

public:
    explicit RandomInternalUpdateManager(unsigned long seed)
        : m_randomGenerator(seed), m_nextThreadIdIndex(0),
          m_nextOtherThreadIdIndex(0) {}
};

class InteractiveInternalUpdateManager : public InternalUpdateManager {
    std::vector<std::vector<std::pair<size_t, Message>>> m_threadIds;

    void
    reset(const StrongReleaseAcquireStorageManager &storageManager) override;

    std::optional<std::pair<size_t, size_t>> getThreadIds() override;

public:
};

} // namespace wmm::storage::SRA
