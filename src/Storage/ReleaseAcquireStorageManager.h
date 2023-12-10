//
// Created by veronika on 06.12.23.
//

#pragma once

#include "Storage.h"
#include "StorageManager.h"

#include <map>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wmm::storage::RA {

class View {
    std::vector<double> m_timestamps;

public:
    explicit View(size_t n, double value = 0) : m_timestamps(n, value) {}

    View &operator|=(const View &view);
    View &operator&=(const View &view);

    friend View operator|(View lhs, const View &rhs) { return lhs |= rhs; }
    friend View operator&(View lhs, const View &rhs) { return lhs &= rhs; }

    double operator[](size_t i) const { return m_timestamps[i]; };

    void setTimestamp(size_t location, double timestamp) {
        m_timestamps[location] = timestamp;
    }

    [[nodiscard]] std::string str() const;

    [[nodiscard]] size_t size() const { return m_timestamps.size(); }
};

struct Message {
private:
    const size_t m_viewSize;

public:
    const size_t location;
    const int32_t value;
    const double timestamp;
    const View baseView;
    const std::optional<View> releaseView;
    bool isUsedByAtomicUpdate;

    Message(size_t location_, int32_t value_, double timestamp_, View baseView_,
            std::optional<View> view_ = {}, bool isUsedByAtomicUpdate = false)
        : m_viewSize(baseView_.size()), location(location_), value(value_),
          timestamp(timestamp_), baseView(std::move(baseView_)),
          releaseView(std::move(view_)),
          isUsedByAtomicUpdate(isUsedByAtomicUpdate) {}

    [[nodiscard]] std::string str() const;

    Message(size_t location, size_t viewSize)
        : Message(location, 0, 0, View(viewSize)) {}

    Message &operator=(Message &&) = delete;
    Message &operator=(const Message &) = delete;
    Message(Message &&) = default;
    Message(const Message &) = default;
};

using MessageRef = std::reference_wrapper<Message>;

class SortedMessageHistory {
private:
    std::map<double, Message> m_buffer;
    size_t m_viewSize;

public:
    void push(const Message &message);
    void pop();
    [[nodiscard]] auto begin() const;
    [[nodiscard]] auto end() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] std::string str() const;

    [[nodiscard]] std::vector<MessageRef>
    filterGreaterOrEqualTimestamp(double timestamp);

    SortedMessageHistory(size_t location, size_t viewSize)
        : m_buffer({{0, Message(location, viewSize)}}), m_viewSize(viewSize) {}

    SortedMessageHistory() = delete;
};

class InternalUpdateManager;
class RandomInternalUpdateManager;
class InteractiveInternalUpdateManager;

using InternalUpdateManagerPtr = std::unique_ptr<InternalUpdateManager>;

enum class Model { RA, SRA };

class ReleaseAcquireStorageManager : public StorageManagerInterface {
private:
    size_t m_storageSize;
    size_t m_viewSize;
    std::vector<View> m_threadViews;
    std::vector<View> m_baseViewPerThread;
    InternalUpdateManagerPtr m_internalUpdateManager;
    std::vector<SortedMessageHistory> m_messages;
    Model m_model;

    void write(size_t threadId, size_t location, int32_t value,
               bool useMinTimestamp, bool withRelease);
    int32_t read(size_t threadId, size_t location, bool withAcquire,
                 bool isReadBeforeAtomicUpdate);

    [[nodiscard]] std::vector<MessageRef>
    availableMessages(size_t threadId, size_t location);

    void applyMessage(size_t threadId, const Message &message,
                      bool withAcquire);
    void cleanUpHistory(size_t location);
    [[nodiscard]] double minTimestamp(size_t location) const;

public:
    explicit ReleaseAcquireStorageManager(
            size_t storageSize, size_t nOfThreads, Model model,
            InternalUpdateManagerPtr &&internalUpdateManager,
            storage::LoggerPtr &&logger = std::make_unique<FakeStorageLogger>())
        : StorageManagerInterface(std::move(logger)),
          m_storageSize(storageSize), m_viewSize(storageSize + 1),
          m_threadViews(nOfThreads, View(m_viewSize)),
          m_baseViewPerThread(nOfThreads, View(m_viewSize)),
          m_internalUpdateManager(std::move(internalUpdateManager)),
          m_model(model) {
        m_messages.reserve(m_viewSize);
        for (size_t loc = 0; loc < m_viewSize; ++loc) {
            m_messages.emplace_back(loc, m_viewSize);
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

    bool internalUpdate() override { return false; }

    friend class RandomInternalUpdateManager;
    friend class InteractiveInternalUpdateManager;
};

class InternalUpdateManager {
    friend class ReleaseAcquireStorageManager;

    [[nodiscard]] virtual Message
    chooseMessage(const std::vector<MessageRef> &messages,
                  bool markReadBeforeAtomicUpdate) const = 0;
    [[nodiscard]] virtual double
    chooseNewTimestamp(const std::vector<MessageRef> &messages) const = 0;

public:
    virtual ~InternalUpdateManager() = default;
};

class RandomInternalUpdateManager : public InternalUpdateManager {
    mutable std::mt19937 m_randomGenerator;

    [[nodiscard]] Message
    chooseMessage(const std::vector<MessageRef> &messages,
                  bool isReadBeforeAtomicUpdate) const override;
    double
    chooseNewTimestamp(const std::vector<MessageRef> &messages) const override;

public:
    explicit RandomInternalUpdateManager(unsigned long seed)
        : m_randomGenerator(seed) {}
};

class InteractiveInternalUpdateManager : public InternalUpdateManager {

    [[nodiscard]] Message
    chooseMessage(const std::vector<MessageRef> &messages,
                  bool isReadBeforeAtomicUpdate) const override;
    [[nodiscard]] double
    chooseNewTimestamp(const std::vector<MessageRef> &messages) const override;

public:
};

} // namespace wmm::storage::RA
