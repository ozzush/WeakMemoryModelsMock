#pragma once

#include <vector>

#include "../Program/Program.h"
#include "Thread.h"

namespace wmm::execution {

class ThreadManager {
    std::vector<Thread> m_threads;
    storage::StorageManagerPtr m_storageManager;

public:
    ThreadManager(const std::vector<program::Program> &programs,
                  storage::StorageManagerPtr storageManager,
                  size_t threadLocalStorageSize);

    bool evaluateThread(size_t threadId);

    [[nodiscard]] bool allThreadsCompleted() const;

    [[nodiscard]] std::vector<storage::Storage> getThreadLocalStorages() const;

    [[nodiscard]] std::vector<size_t> unfinishedThreads() const;
};

} // namespace wmm
