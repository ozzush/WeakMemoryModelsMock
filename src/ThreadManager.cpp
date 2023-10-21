#include "ThreadManager.h"

namespace wmm {

ThreadManager::ThreadManager(const std::vector<Program> &programs,
                             StorageManagerPtr storageManager,
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

std::vector<Storage> ThreadManager::getThreadLocalStorages() const {
    std::vector<Storage> storages;
    storages.reserve(m_threads.size());
    for (const auto &thread: m_threads) {
        storages.push_back(thread.getLocalStorage());
    }
    return storages;
}

} // namespace wmm