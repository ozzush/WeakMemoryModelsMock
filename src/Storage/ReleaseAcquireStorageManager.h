//
// Created by veronika on 06.12.23.
//

#pragma once

#include "Storage.h"
#include "StorageManager.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <set>
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

    friend bool operator==(const View &lhs, const View &rhs) {
        return lhs.m_timestamps == lhs.m_timestamps;
    }
    friend bool operator!=(const View &lhs, const View &rhs) {
        return lhs.m_timestamps != lhs.m_timestamps;
    }

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

    Message(size_t location_, int32_t value_, double timestamp_, View baseView_,
            std::optional<View> view_ = {})
        : m_viewSize(baseView_.size()), location(location_), value(value_),
          timestamp(timestamp_), baseView(std::move(baseView_)),
          releaseView(std::move(view_)) {}

    [[nodiscard]] std::string str() const;

    Message(size_t location, size_t viewSize)
        : Message(location, 0, 0, View(viewSize)) {}

    explicit Message(double timestamp_)
        : m_viewSize(0), location(0), value(0), timestamp(timestamp_),
          baseView(m_viewSize) {}

    Message &operator=(Message &&) = delete;
    Message &operator=(const Message &) = delete;
    Message(Message &&) = default;
    Message(const Message &) = default;

    friend bool operator<(const Message &lhs, const Message &rhs) {
        return lhs.timestamp < rhs.timestamp;
    }
    //
    //    friend bool operator==(const Message &lhs, const Message &rhs) {
    //        return lhs.timestamp == rhs.timestamp;
    //    }
};

class SortedMessageHistory {
private:
    std::set<Message> m_buffer;
    size_t m_viewSize;

public:
    void push(const Message &message);
    void pop();
    [[nodiscard]] auto begin() const;
    [[nodiscard]] auto end() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] std::string str() const;

    [[nodiscard]] std::vector<Message>
    filterGreaterOrEqualTimestamp(double timestamp) const;

    SortedMessageHistory(size_t location, size_t viewSize)
        : m_buffer({Message(location, viewSize)}), m_viewSize(viewSize) {}

    SortedMessageHistory() = delete;

    //    Message operator[](double timestamp) const { return *m_buffer.find(Message(timestamp));}
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
               bool useMinTimestamp = false, bool withRelease = true);
    int32_t read(size_t threadId, size_t location, bool withAcquire = true);

    [[nodiscard]] std::vector<Message> availableMessages(size_t threadId,
                                                         size_t location) const;

    void applyMessage(size_t threadId, const Message &message, bool withAcquire);
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
    chooseMessage(const std::vector<Message> &messages) const = 0;
    [[nodiscard]] virtual double
    chooseNewTimestamp(const std::vector<Message> &messages) const = 0;

public:
    virtual ~InternalUpdateManager() = default;
};

class RandomInternalUpdateManager : public InternalUpdateManager {
    mutable std::mt19937 m_randomGenerator;

    [[nodiscard]] Message
    chooseMessage(const std::vector<Message> &messages) const override;
    double
    chooseNewTimestamp(const std::vector<Message> &messages) const override;

public:
    explicit RandomInternalUpdateManager(unsigned long seed)
        : m_randomGenerator(seed) {}
};

class InteractiveInternalUpdateManager : public InternalUpdateManager {

    [[nodiscard]] Message
    chooseMessage(const std::vector<Message> &messages) const override;
    [[nodiscard]] double
    chooseNewTimestamp(const std::vector<Message> &messages) const override;

public:
};

} // namespace wmm::storage::RA
