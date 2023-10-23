#pragma once

#include <vector>

#include "Instructions.h"
#include "Program.h"
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
    void evaluateThreadLocalInstructions(size_t threadId);
    [[nodiscard]] bool allThreadsCompleted() const;
    [[nodiscard]] std::vector<storage::Storage> getThreadLocalStorages() const;
    [[nodiscard]] std::vector<size_t> unfinishedThreads() const;
    [[nodiscard]] size_t size() const;

    [[nodiscard]] std::shared_ptr<program::Instruction>
    getCurrentInstructionForThread(size_t threadId) const;

    [[nodiscard]] const storage::Storage &
    getThreadLocalStorage(size_t threadId) const;
};

} // namespace wmm::execution
