#pragma once

#include "Thread.h"
#include <vector>

namespace wmm {

class ThreadManager {
    std::vector<Thread> m_threads;
    StorageManagerPtr m_storageManager;

public:
    ThreadManager(const std::vector<Program> &programs,
                  StorageManagerPtr storageManager,
                  size_t threadLocalStorageSize);

    bool evaluateThread(size_t threadId);

    [[nodiscard]] bool allThreadsCompleted() const;

    [[nodiscard]] std::vector<Storage> getThreadLocalStorages() const;

    [[nodiscard]] std::vector<size_t> unfinishedThreads() const;
};

} // namespace wmm
