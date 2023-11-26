//
// Created by veronika on 14.11.23.
//

#include "StrongReleaseAcquireStorageManager.h"
#include "ReleaseAcquireStorageManager.h"

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <sstream>

namespace wmm::storage::SRA {

namespace {
template<class T>
std::string join(const std::vector<T> &data,
                 std::function<std::string(T)> toString,
                 const std::string &separator = " ") {
    std::stringstream stream;
    bool isFirstIteration;
    for (const auto &elm: data) {
        if (!isFirstIteration) { stream << separator; }
        stream << toString(elm);
    }
    return stream.str();
}

template<class T>
std::string join(const std::vector<T> &data,
                 const std::string &separator = " ") {
    std::stringstream stream;
    bool isFirstIteration;
    for (const auto &elm: data) {
        if (!isFirstIteration) { stream << separator; }
        stream << elm;
    }
    return stream.str();
}
} // namespace

bool StrongReleaseAcquireStorageManager::internalUpdate() {
    m_internalUpdateManager->reset(*this);
    while (auto threadIds = m_internalUpdateManager->getThreadIds()) {
        size_t threadId = threadIds.value().first;
        size_t otherThreadId = threadIds.value().second;
        if (writeFromBuffer(threadId, otherThreadId)) { return true; }
    }
    return false;
}

bool StrongReleaseAcquireStorageManager::writeFromBuffer(size_t threadId,
                                                         size_t otherThreadId) {
    auto &thread = m_threads[threadId];
    const auto &otherThread = m_threads[otherThreadId];
    size_t bufferPosition = thread.getBufferPos(otherThreadId);
    auto message = otherThread.readMessage(bufferPosition);
    if (!message) { return false; }
    thread.advanceBufferPos(otherThreadId);
    cleanUpBuffer(otherThreadId);
    auto messageVal = message.value();
    if (thread.write(messageVal)) {
        m_storageLogger->info(
                std::format("INTERNAL: t{} execute from t{}: {}", threadId,
                            otherThreadId, message->str()));
    } else {
        m_storageLogger->info(
                std::format("INTERNAL: t{} skip message from t{}: {}", threadId,
                            otherThreadId, message->str()));
    }
    return true;
}

void StrongReleaseAcquireStorageManager::write(size_t threadId, size_t location,
                                               int32_t value) {
    size_t timestamp = ++m_locationTimestamps[location];
    m_threads[threadId].write(location, value, timestamp);
}

void StrongReleaseAcquireStorageManager::writeStorage(
        std::ostream &outputStream) const {
    size_t i = 0;
    for (const auto &thread: m_threads) {
        outputStream << "Thread " << i << '\n' << thread.str() << '\n';
    }
    outputStream << "Location timestamps: " << join(m_locationTimestamps);
    outputStream << '\n';
}

int32_t StrongReleaseAcquireStorageManager::load(size_t threadId,
                                                 size_t address,
                                                 MemoryAccessMode accessMode) {
    int32_t value = m_threads[threadId].read(address);
    m_storageLogger->load(threadId, address, accessMode, value);
    return value;
}

void StrongReleaseAcquireStorageManager::store(size_t threadId, size_t address,
                                               int32_t value,
                                               MemoryAccessMode accessMode) {
    m_storageLogger->store(threadId, address, value, accessMode);
    write(threadId, address, value);
}

void StrongReleaseAcquireStorageManager::compareAndSwap(
        size_t threadId, size_t address, int32_t expectedValue,
        int32_t newValue, MemoryAccessMode accessMode) {
    while (m_threads[threadId].timestamp(address) <
           m_locationTimestamps[address]) {
        m_storageLogger->compareAndSwap(threadId, address, expectedValue, {},
                                        newValue, accessMode);
        if (!internalUpdate()) {
            throw std::runtime_error("Execution is stuck");
        }
    }
    int32_t value = m_threads[threadId].read(address);
    m_storageLogger->compareAndSwap(threadId, address, expectedValue, value,
                                    newValue, accessMode);
    if (value == expectedValue) { write(threadId, address, newValue); }
}
void StrongReleaseAcquireStorageManager::fetchAndIncrement(
        size_t threadId, size_t address, int32_t increment,
        MemoryAccessMode accessMode) {
    while (m_threads[threadId].timestamp(address) <
           m_locationTimestamps[address]) {
        if (!internalUpdate()) {
            throw std::runtime_error("Execution is stuck");
        }
    }
    auto value = m_threads[threadId].read(address);
    write(threadId, address, value + increment);
}
void StrongReleaseAcquireStorageManager::fence(size_t threadId,
                                               MemoryAccessMode accessMode) {
    throw std::runtime_error("Fences not implemented");
}
size_t StrongReleaseAcquireStorageManager::minBufferPos(size_t threadId) const {
    size_t minPosition = SIZE_MAX;
    for (const auto &m_thread: m_threads) {
        minPosition = std::min(minPosition, m_thread.getBufferPos(threadId));
    }
    return minPosition;
}

void StrongReleaseAcquireStorageManager::cleanUpBuffer(size_t threadId) {
    size_t minPosition = minBufferPos(threadId);
    if (minPosition == 0) { return; }
    m_threads[threadId].popNFromBuffer(minPosition);
    for (auto &thread: m_threads) {
        thread.resetBufferPosByN(threadId, minPosition);
    }
}

void RandomInternalUpdateManager::reset(
        const StrongReleaseAcquireStorageManager &storageManager) {
    m_threadIds.resize(storageManager.m_threads.size());
    m_otherThreadIds.resize(storageManager.m_threads.size());
    for (int i = 0; i < m_threadIds.size(); ++i) {
        m_threadIds[i] = i;
        m_otherThreadIds[i] = i;
    }
    std::shuffle(m_threadIds.begin(), m_threadIds.end(), m_randomGenerator);
    std::shuffle(m_otherThreadIds.begin(), m_otherThreadIds.end(),
                 m_randomGenerator);
    m_nextThreadIdIndex = 0;
    m_nextOtherThreadIdIndex = 0;
}

std::optional<std::pair<size_t, size_t>>
RandomInternalUpdateManager::getThreadIds() {
    if (m_nextOtherThreadIdIndex >= m_otherThreadIds.size()) {
        ++m_nextThreadIdIndex;
        m_nextOtherThreadIdIndex = 0;
    }
    if (m_nextThreadIdIndex < m_threadIds.size()) {
        auto threadId = m_threadIds[m_nextThreadIdIndex];
        auto otherThreadId = m_otherThreadIds[m_nextOtherThreadIdIndex++];
        if (threadId == otherThreadId) { return getThreadIds(); }
        return {{threadId, otherThreadId}};
    }
    return {};
}
//
//std::optional<std::pair<size_t, size_t>>
//InteractiveInternalUpdateManager::getThreadIds() {
//    if (m_threadIds.empty()) {
//        std::cout << "All buffers are empty.\n";
//        return {};
//    }
//    std::cout << "Choose buffer to propagate:\n";
//    for (size_t i = 0; i < m_threadIds.size(); ++i) {
//        std::cout << std::format("{}: {}\n", m_threadIds[i],
//                                 m_buffers[i].get().str());
//    }
//    while (true) {
//        size_t threadId;
//        std::cout << "Enter buffer id > ";
//        std::cin >> threadId;
//        if (std::cin.eof() || std::cin.fail()) { return {}; }
//        if (std::count(m_threadIds.begin(), m_threadIds.end(), threadId) == 0) {
//            std::cout << "This buffer cannot be propagated.\n";
//        } else {
//            return threadId;
//        }
//    }
//}
//
//void InteractiveInternalUpdateManager::reset(
//        const StrongReleaseAcquireStorageManager &storageManager) {
//    m_threadIds.clear();
//    m_buffers.clear();
//    for (size_t i = 0; i < storageManager.m_threads.size(); ++i) {
//        if (!storageManager.m_threads[i].empty()) {
//            m_threadIds.push_back(i);
//            m_buffers.emplace_back(storageManager.m_threads.at(i));
//        }
//    }
//}

void MessageBuffer::push(Message message) { m_buffer.push_back(message); }

bool MessageBuffer::empty() const { return m_buffer.empty(); }

std::string MessageBuffer::str() const {
    std::string result;
    bool isFirstIteration = true;
    for (auto message: m_buffer) {
        if (!isFirstIteration) result += ' ';
        result += message.str();
        isFirstIteration = false;
    }
    return result;
}

Message MessageBuffer::operator[](size_t i) const { return m_buffer[i]; }

size_t MessageBuffer::size() const { return m_buffer.size(); }

void MessageBuffer::popN(size_t n) {
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<long>(n));
}

std::string Message::str() const {
    return std::format("#{}->{} @{}", location, value, timestamp);
}

std::optional<Message> ThreadState::readMessage(size_t i) const {
    if (i >= m_outgoingBuffer.size()) { return {}; }
    return m_outgoingBuffer[i];
}

bool ThreadState::write(size_t location, int32_t value, size_t timestamp) {
    return write(Message(location, value, timestamp));
}

size_t ThreadState::advanceBufferPos(size_t threadId) {
    return m_bufferPositions[threadId]++;
}

size_t ThreadState::getBufferPos(size_t threadId) const {
    return m_bufferPositions[threadId];
}

bool ThreadState::write(Message message) {
    size_t location = message.location;
    int32_t value = message.value;
    size_t timestamp = message.timestamp;
    if (timestamp <= m_locationTimestamps[location]) {
        return false;
    }
    m_localStorage.store(location, value);
    m_outgoingBuffer.push(message);
    ++m_bufferPositions[m_threadId];
    m_locationTimestamps[location] = timestamp;
    return true;
}

std::string ThreadState::str() const {
    return std::format("Storage: {}\nBuffer: {}\nBuffer positions: {}\nLast "
                       "timestamps: {}",
                       m_localStorage.str(), m_outgoingBuffer.str(),
                       join(m_bufferPositions), join(m_locationTimestamps));
}

std::optional<std::pair<size_t, size_t>>
InteractiveInternalUpdateManager::getThreadIds() {
    bool allBuffersAreRead = true;
    for (const auto &threadIds: m_threadIds) {
        if (!threadIds.empty()) {
            allBuffersAreRead = false;
            break;
        }
    }
    if (allBuffersAreRead) {
        std::cout << "No execution from other thread's buffer is possible\n";
        return {};
    }
    std::cout << "Choose a pair of threads. First thread will execute a "
                 "message from the second thread's buffer:\n";
    for (size_t i = 0; i < m_threadIds.size(); ++i) {
        std::function<std::string(std::pair<size_t, Message>)>
                threadIdAndMessageToString =
                        [](std::pair<size_t, Message> pair) -> std::string {
            return std::format("{} ({})", pair.first, pair.second.str());
        };
        std::cout << std::format(
                "{}: {}\n", i,
                join(m_threadIds[i], threadIdAndMessageToString), " | ");
    }
    while (true) {
        size_t threadId, otherThreadId;
        std::cout << "Enter thread ids > ";
        std::cin >> threadId >> otherThreadId;
        if (std::cin.eof() || std::cin.fail()) { return {}; }
        if (threadId > m_threadIds.size()) {
            std::cout << "Index out of bounds\n";
        }
        for (auto [id, message]: m_threadIds[threadId]) {
            if (id == otherThreadId) { return {{threadId, otherThreadId}}; }
        }
        std::cout << "Choose a valid index\n";
    }
}

void InteractiveInternalUpdateManager::reset(
        const StrongReleaseAcquireStorageManager &storageManager) {
    const auto &threads = storageManager.m_threads;
    m_threadIds.clear();
    for (const auto &thread: threads) {
        m_threadIds.emplace_back();
        for (size_t threadId = 0; threadId < threads.size(); ++threadId) {
            if (threadId == thread.m_threadId) continue;
            auto message = threads[threadId].readMessage(
                    thread.getBufferPos(threadId));
            if (message) {
                m_threadIds.back().emplace_back(threadId, message.value());
            }
        }
    }
}
} // namespace wmm::storage::SRA