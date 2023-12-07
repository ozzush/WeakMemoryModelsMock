//
// Created by veronika on 06.12.23.
//

#include "ReleaseAcquireStorageManager.h"
#include "Util.h"

#include <format>
#include <iostream>

namespace wmm::storage::RA {

namespace {
double middleTimestamp(const Message &lhs, const Message &rhs) {
    return (lhs.timestamp + rhs.timestamp) / 2;
}

bool isRelease(MemoryAccessMode accessMode) {
    return accessMode == MemoryAccessMode::Release || accessMode == MemoryAccessMode::ReleaseAcquire;
}

bool isAcquire(MemoryAccessMode accessMode) {
    return accessMode == MemoryAccessMode::Acquire || accessMode == MemoryAccessMode::ReleaseAcquire;
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
    return std::format("<#{}->{} @{} view: {}>", location, value, timestamp,
                       view.str());
}


void SortedMessageHistory::push(const Message &message) {
    m_buffer.insert(message);
}

bool SortedMessageHistory::empty() const { return m_buffer.empty(); }

std::string SortedMessageHistory::str() const {
    std::string result;
    bool isFirstIteration = true;
    for (const auto &message: m_buffer) {
        if (!isFirstIteration) result += ' ';
        result += message.str();
        isFirstIteration = false;
    }
    return result;
}

//Message SortedMessageHistory::operator[](size_t i) const { return m_buffer[i]; }

size_t SortedMessageHistory::size() const { return m_buffer.size(); }

void SortedMessageHistory::pop() { m_buffer.erase(m_buffer.begin()); }
auto SortedMessageHistory::begin() const { return m_buffer.begin(); }
auto SortedMessageHistory::end() const { return m_buffer.end(); }

std::vector<Message>
SortedMessageHistory::filterGreaterOrEqualTimestamp(double timestamp) const {
    std::vector<Message> res;
    auto begin = m_buffer.lower_bound(Message(timestamp));
    for (auto it = begin; it != m_buffer.end(); ++it) { res.push_back(*it); }
    return res;
}

std::vector<Message>
ReleaseAcquireStorageManager::availableMessages(size_t threadId,
                                                size_t location) const {
    double minTimestamp = m_threadViews[threadId][location];
    return m_messages[location].filterGreaterOrEqualTimestamp(minTimestamp);
}
void ReleaseAcquireStorageManager::applyMessage(size_t threadId,
                                                const Message &message) {
    m_threadViews[threadId] |= message.view;
    cleanUpHistory(message.location);
}

void ReleaseAcquireStorageManager::cleanUpHistory(size_t location) {
    double minTime = minTimestamp(location);
    auto &buffer = m_messages[location];
    while (!buffer.empty() && buffer.begin()->timestamp < minTime) {
        buffer.pop();
    }
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
                                         bool release) {
    auto messages = availableMessages(threadId, location);
    double minPossibleTimestamp =
            (messages.size() < 2) ? messages.back().timestamp + 1
                                  : middleTimestamp(messages[0], messages[1]);
    double newTimestamp =
            (m_model == Model::SRA) ? messages.back().timestamp + 1
            : (useMinTimestamp)
                    ? minPossibleTimestamp
                    : m_internalUpdateManager->chooseNewTimestamp(messages);
    auto newView = m_threadViews[threadId];
    newView.setTimestamp(location, newTimestamp);
    Message message{location, value, newTimestamp, newView};
    if (release) { m_threadViews[threadId] = newView; }
    m_messages[location].push(message);
}

int32_t ReleaseAcquireStorageManager::load(size_t threadId, size_t address,
                                           MemoryAccessMode accessMode) {
    auto value = read(threadId, address,
                      (accessMode == MemoryAccessMode::Acquire ||
                       accessMode == MemoryAccessMode::ReleaseAcquire));
    m_storageLogger->load(threadId, address, accessMode, value);
    return value;
}

void ReleaseAcquireStorageManager::store(size_t threadId, size_t address,
                                         int32_t value,
                                         MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    write(threadId, address, value);
}

void ReleaseAcquireStorageManager::compareAndSwap(size_t threadId,
                                                  size_t address,
                                                  int32_t expectedValue,
                                                  int32_t newValue,
                                                  MemoryAccessMode accessMode) {
    int32_t value = read(threadId, address);
    if (value == expectedValue) { write(threadId, address, newValue, true); }
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, value,
                                    newValue, accessMode);
}

int32_t ReleaseAcquireStorageManager::read(size_t threadId, size_t location,
                                           bool withViewUpdate) {
    auto messages = availableMessages(threadId, location);
    auto message = m_internalUpdateManager->chooseMessage(messages);
    if (withViewUpdate) { applyMessage(threadId, message); }
    return message.value;
}

void ReleaseAcquireStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    int32_t value = read(threadId, address);
    write(threadId, address, value + increment, true);
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
    if (isAcquire(accessMode)) {
        read(threadId, m_storageSize);
    }
    if (isRelease(accessMode)) {
        write(threadId, m_storageSize, 0, true);
    }
}

Message RandomInternalUpdateManager::chooseMessage(
        const std::vector<Message> &messages) const {
    if (messages.empty()) {
        throw std::runtime_error("No messages to read from");
    }
    std::uniform_int_distribution<size_t> distribution(0, messages.size() - 1);
    return messages[distribution(m_randomGenerator)];
}
double RandomInternalUpdateManager::chooseNewTimestamp(
        const std::vector<Message> &messages) const {
    if (messages.empty()) {
        throw std::runtime_error("No messages to base a timestamp");
    }
    std::uniform_int_distribution<size_t> distribution(0, messages.size() - 1);
    size_t pos = distribution(m_randomGenerator);
    if (pos == messages.size() - 1) {
        return messages.back().timestamp + 1;
    } else {
        return middleTimestamp(messages[pos], messages[pos + 1]);
    }
}
Message InteractiveInternalUpdateManager::chooseMessage(
        const std::vector<Message> &messages) const {
    if (messages.empty()) {
        throw std::runtime_error("No messages to read from");
    }
    std::cout << "Available messages to read from:\n";
    for (size_t i = 0; i < messages.size(); ++i) {
        std::cout << std::format("[{}]: {}\n", i, messages[i].str());
    }
    size_t i = SIZE_MAX;
    while (true) {
        std::cout << "Choose message: ";
        if (!(std::cin >> i)) { throw std::runtime_error("End of input"); }
        if (i >= messages.size()) {
            std::cout << std::format("Input integer in [0, {}]\n",
                                     messages.size() - 1);
            continue;
        }
        break;
    }
    return messages[i];
}

double InteractiveInternalUpdateManager::chooseNewTimestamp(
        const std::vector<Message> &messages) const {
    if (messages.empty()) {
        throw std::runtime_error("No messages to base a timestamp");
    }
    std::cout << "Available messages:\n";
    for (size_t i = 0; i < messages.size(); ++i) {
        std::cout << std::format("{}\n", i, messages[i].str());
    }
    double t = NAN;
    std::cout << std::format(
            "Choose a floating-point timestamp greater than {} that doesn't "
            "collide with any existing timestamp\n",
            messages.front().timestamp);
    while (true) {
        std::cout << "Choose timestamp: ";
        if (!(std::cin >> t)) { throw std::runtime_error("End of input"); }
        // TODO: validate input
        break;
    }
    return t;
}
} // namespace wmm::storage::RA