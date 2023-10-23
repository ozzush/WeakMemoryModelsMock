#include "ThreadManager.h"
#include <algorithm>

namespace wmm::executor {

ThreadManager::ThreadManager(const std::vector<program::Program> &programs,
                             storage::StorageManagerPtr storageManager,
                             size_t threadLocalStorageSize)
    : m_storageManager(std::move(storageManager)) {
    m_threads.reserve(programs.size());
    for (const auto &program: programs) {
        m_threads.emplace_back(program, m_storageManager, m_threads.size(),
                               threadLocalStorageSize);
    }
}

bool ThreadManager::evaluateThread(size_t threadId) {
    return m_threads.at(threadId).evaluateInstruction();
}

bool ThreadManager::allThreadsCompleted() const {
    return std::all_of(m_threads.begin(), m_threads.end(),
                       [](const auto &thread) { return thread.isFinished(); });
}

std::vector<storage::Storage> ThreadManager::getThreadLocalStorages() const {
    std::vector<storage::Storage> storages;
    storages.reserve(m_threads.size());
    for (const auto &thread: m_threads) {
        storages.push_back(thread.getLocalStorage());
    }
    return storages;
}

std::vector<size_t> ThreadManager::unfinishedThreads() const {
    std::vector<size_t> unfinishedThreads;
    for (const auto &thread : m_threads) {
        if (!thread.isFinished()) {
            unfinishedThreads.push_back(thread.id);
        }
    }
    return unfinishedThreads;
}

} // namespace wmm