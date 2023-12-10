//
// Created by veronika on 06.12.23.
//

#include "ReleaseAcquireStorageManager.h"
#include "Util.h"

#include <cassert>
#include <format>
#include <iostream>

namespace wmm::storage::RA {

namespace {
double middleTimestamp(const Message &lhs, const Message &rhs) {
    return (lhs.timestamp + rhs.timestamp) / 2;
}

bool isRelease(MemoryAccessMode accessMode) {
    return accessMode == MemoryAccessMode::Release ||
           accessMode == MemoryAccessMode::ReleaseAcquire;
}

bool isAcquire(MemoryAccessMode accessMode) {
    return accessMode == MemoryAccessMode::Acquire ||
           accessMode == MemoryAccessMode::ReleaseAcquire;
}

size_t
countMessagesNotUsedInAtomicUpdates(const std::vector<MessageRef> &messages) {
    size_t count = 0;
    for (const auto &message: messages) {
        if (!message.get().isUsedByAtomicUpdate) { ++count; }
    }
    return count;
}

std::vector<MessageRef>
filterMessagesNotUsedInAtomicUpdates(const std::vector<MessageRef> &messages) {
    std::vector<MessageRef> result;
    for (const auto &message: messages) {
        if (!message.get().isUsedByAtomicUpdate) { result.push_back(message); }
    }
    return result;
}


size_t findPosOfNthMessageNotUsedInAtomicUpdates(
        size_t n, const std::vector<MessageRef> &messages) {
    size_t count = 0;
    for (size_t i = 0; i < messages.size(); ++i) {
        if (messages[i].get().isUsedByAtomicUpdate) { continue; }
        ++count;
        if (count == n) { return i; }
    }
    assert(false);
}
} // namespace

View &View::operator|=(const View &view) {
    for (size_t i = 0; i < m_timestamps.size(); ++i) {
        if (m_timestamps[i] < view.m_timestamps[i]) {
            m_timestamps[i] = view.m_timestamps[i];
        }
    }
    return *this;
}

std::string View::str() const { return util::join(m_timestamps); }

View &View::operator&=(const View &view) {
    for (size_t i = 0; i < m_timestamps.size(); ++i) {
        if (m_timestamps[i] > view.m_timestamps[i]) {
            m_timestamps[i] = view.m_timestamps[i];
        }
    }
    return *this;
}

std::string Message::str() const {
    std::string releaseViewStr = (releaseView) ? releaseView->str() : "None";
    return std::format("<#{}->{} @{} is_used_by_atomic_update: {} base_view: "
                       "{} release_view: {}>",
                       location, value, timestamp,
                       isUsedByAtomicUpdate ? "TRUE" : "FALSE", baseView.str(),
                       releaseViewStr);
}


void SortedMessageHistory::push(const Message &message) {
    m_buffer.insert({message.timestamp, message});
}

bool SortedMessageHistory::empty() const { return m_buffer.empty(); }

std::string SortedMessageHistory::str() const {
    std::string result;
    bool isFirstIteration = true;
    for (const auto &message: m_buffer) {
        if (!isFirstIteration) result += ' ';
        result += message.second.str();
        isFirstIteration = false;
    }
    return result;
}

size_t SortedMessageHistory::size() const { return m_buffer.size(); }

void SortedMessageHistory::pop() { m_buffer.erase(m_buffer.begin()); }
auto SortedMessageHistory::begin() const { return m_buffer.begin(); }
auto SortedMessageHistory::end() const { return m_buffer.end(); }

std::vector<MessageRef>
SortedMessageHistory::filterGreaterOrEqualTimestamp(double timestamp) {
    std::vector<MessageRef> res;
    auto begin = m_buffer.lower_bound(timestamp);
    for (auto it = begin; it != m_buffer.end(); ++it) {
        res.push_back(std::ref(it->second));
    }
    return res;
}

std::vector<MessageRef>
ReleaseAcquireStorageManager::availableMessages(size_t threadId,
                                                size_t location) {
    double minTimestamp = m_threadViews[threadId][location];
    return m_messages[location].filterGreaterOrEqualTimestamp(minTimestamp);
}
void ReleaseAcquireStorageManager::applyMessage(size_t threadId,
                                                const Message &message,
                                                bool withAcquire) {
    m_threadViews[threadId] |= (withAcquire && message.releaseView)
                                       ? message.releaseView.value()
                                       : message.baseView;
    cleanUpHistory(message.location);
}

void ReleaseAcquireStorageManager::cleanUpHistory(size_t location) {
    double minTime = minTimestamp(location);
    auto &buffer = m_messages[location];
    while (buffer.begin()->first < minTime) { buffer.pop(); }
}

double ReleaseAcquireStorageManager::minTimestamp(size_t location) const {
    double minTimestamp = INFINITY;
    for (const auto &threadView: m_threadViews) {
        minTimestamp = std::min(minTimestamp, threadView[location]);
    }
    return minTimestamp;
}

void ReleaseAcquireStorageManager::write(size_t threadId, size_t location,
                                         int32_t value, bool useMinTimestamp,
                                         bool withRelease) {
    auto messages = availableMessages(threadId, location);
    double minPossibleTimestamp =
            (messages.size() < 2) ? messages.back().get().timestamp + 1
                                  : middleTimestamp(messages[0], messages[1]);
    double newTimestamp =
            (m_model == Model::SRA) ? messages.back().get().timestamp + 1
            : (useMinTimestamp)
                    ? minPossibleTimestamp
                    : m_internalUpdateManager->chooseNewTimestamp(messages);
    m_threadViews[threadId].setTimestamp(location, newTimestamp);
    std::optional<View> releaseView;
    if (withRelease) {
        m_baseViewPerThread[threadId] = m_threadViews[threadId];
        releaseView = m_threadViews[threadId];
    } else {
        m_baseViewPerThread[threadId].setTimestamp(location, newTimestamp);
    }
    auto baseView = m_baseViewPerThread[threadId];
    Message message{location, value, newTimestamp, baseView, releaseView};
    m_messages[location].push(message);
}

int32_t ReleaseAcquireStorageManager::load(size_t threadId, size_t address,
                                           MemoryAccessMode accessMode) {
    auto value = read(threadId, address, isAcquire(accessMode), false);
    m_storageLogger->load(threadId, address, accessMode, value);
    return value;
}

void ReleaseAcquireStorageManager::store(size_t threadId, size_t address,
                                         int32_t value,
                                         MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    write(threadId, address, value, false, isRelease(accessMode));
}

void ReleaseAcquireStorageManager::compareAndSwap(size_t threadId,
                                                  size_t address,
                                                  int32_t expectedValue,
                                                  int32_t newValue,
                                                  MemoryAccessMode accessMode) {
    int32_t value = read(threadId, address, isAcquire(accessMode), true);
    if (value == expectedValue) {
        write(threadId, address, newValue, true, isRelease(accessMode));
    }
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, value,
                                    newValue, accessMode);
}

int32_t ReleaseAcquireStorageManager::read(size_t threadId, size_t location,
                                           bool withAcquire,
                                           bool isReadBeforeAtomicUpdate) {
    auto messages = availableMessages(threadId, location);
    auto message = m_internalUpdateManager->chooseMessage(
            messages, isReadBeforeAtomicUpdate);
    applyMessage(threadId, message, withAcquire);
    return message.value;
}

void ReleaseAcquireStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    int32_t value = read(threadId, address, isAcquire(accessMode), true);
    write(threadId, address, value + increment, true, isRelease(accessMode));
    m_storageLogger->fetchAndIncrement(threadId, address, increment,
                                       accessMode);
}
void ReleaseAcquireStorageManager::writeStorage(
        std::ostream &outputStream) const {
    size_t location = 0;
    outputStream << "Messages per location:\n";
    for (const auto &locMessages: m_messages) {
        outputStream << std::format("#{}: {}\n", location++, locMessages.str());
    }
    outputStream << "Thread views: \n";
    size_t threadId = 0;
    for (const auto &threadView: m_threadViews) {
        outputStream << std::format("#{}: {}\n", threadId++, threadView.str());
    }
}
void ReleaseAcquireStorageManager::fence(size_t threadId,
                                         MemoryAccessMode accessMode) {
    m_storageLogger->fence(threadId, accessMode);
    read(threadId, m_storageSize, isAcquire(accessMode), true);
    write(threadId, m_storageSize, 0, true, isRelease(accessMode));
}

Message RandomInternalUpdateManager::chooseMessage(
        const std::vector<MessageRef> &messages,
        bool isReadBeforeAtomicUpdate) const {
    assert(!messages.empty());
    if (isReadBeforeAtomicUpdate) {
        size_t messagesNotUsedInAtomicUpdates =
                countMessagesNotUsedInAtomicUpdates(messages);
        std::uniform_int_distribution<size_t> distribution(
                1, messagesNotUsedInAtomicUpdates);
        size_t pos = distribution(m_randomGenerator);
        auto baseMessagePos =
                findPosOfNthMessageNotUsedInAtomicUpdates(pos, messages);
        messages[baseMessagePos].get().isUsedByAtomicUpdate = true;
        return messages[baseMessagePos].get();
    } else {
        std::uniform_int_distribution<size_t> distribution(0,
                                                           messages.size() - 1);
        return messages[distribution(m_randomGenerator)].get();
    }
}
double RandomInternalUpdateManager::chooseNewTimestamp(
        const std::vector<MessageRef> &messages) const {
    assert(!messages.empty());
    size_t countOfMessagesToBaseATimestampOn =
            countMessagesNotUsedInAtomicUpdates(messages);
    std::uniform_int_distribution<size_t> distribution(
            1, countOfMessagesToBaseATimestampOn);
    size_t pos = distribution(m_randomGenerator);
    auto baseMessagePos =
            findPosOfNthMessageNotUsedInAtomicUpdates(pos, messages);
    if (baseMessagePos == messages.size() - 1) {
        return messages.back().get().timestamp + 1;
    } else {
        return middleTimestamp(messages[baseMessagePos],
                               messages[baseMessagePos + 1]);
    }
}
Message InteractiveInternalUpdateManager::chooseMessage(
        const std::vector<MessageRef> &messages,
        bool isReadBeforeAtomicUpdate) const {
    // TODO: Not sure if binding to an rvalue is legal here
    const auto &availableMessages =
            (isReadBeforeAtomicUpdate)
                    ? filterMessagesNotUsedInAtomicUpdates(messages)
                    : messages;
    assert(!availableMessages.empty());
    std::cout << "Available messages to read from:\n";
    for (size_t i = 0; i < availableMessages.size(); ++i) {
        std::cout << std::format("[{}]: {}\n", i,
                                 availableMessages[i].get().str());
    }
    size_t i = SIZE_MAX;
    while (true) {
        std::cout << "Choose message: ";
        if (!(std::cin >> i)) { throw std::runtime_error("End of input"); }
        if (i >= availableMessages.size()) {
            std::cout << std::format("Input integer in [0, {}]\n",
                                     availableMessages.size() - 1);
            continue;
        }
        break;
    }
    return availableMessages[i].get();
}

double InteractiveInternalUpdateManager::chooseNewTimestamp(
        const std::vector<MessageRef> &messages) const {
    assert(!messages.empty());
    std::cout << "Available messages:\n";
    for (size_t i = 0; i < messages.size(); ++i) {
        std::cout << std::format("{}\n", i, messages[i].get().str());
    }
    double t = NAN;
    std::cout << std::format(
            "Choose a floating-point timestamp greater than {} that doesn't "
            "collide with any existing timestamp and doesn't go immediately "
            "after "
            "a message used by an atomic update\n",
            messages.front().get().timestamp);
    while (true) {
        std::cout << "Choose timestamp: ";
        if (!(std::cin >> t)) { throw std::runtime_error("End of input"); }
        // TODO: validate input
        break;
    }
    return t;
}
} // namespace wmm::storage::RA