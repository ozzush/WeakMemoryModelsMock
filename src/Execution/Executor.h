//
// Created by veronika on 22.10.23.
//

#pragma once

#include <algorithm>
#include <random>

#include "ThreadManager.h"

namespace wmm::execution {

class ExecutorInterface {
protected:
    ThreadManager m_threadManager;
    storage::StorageManagerPtr m_storageManager;

    virtual bool executeThread() = 0;
    virtual bool executeInternalMemoryUpdate() { return m_storageManager->internalUpdate(); };

public:
    virtual bool execute() = 0;

    ExecutorInterface(const std::vector<program::Program> &programs,
                      const storage::StorageManagerPtr &storageManager,
                      size_t threadLocalStorageSize)
        : m_storageManager(storageManager),
          m_threadManager(programs, storageManager, threadLocalStorageSize) {}



    virtual void writeState(std::ostream &outputStream) const = 0;
};

class RandomExecutor : public ExecutorInterface {
    std::mt19937 m_randomGenerator;

    bool executeThread() override;

public:
    RandomExecutor(const std::vector<program::Program> &programs,
                   const storage::StorageManagerPtr &storageManager,
                   size_t threadLocalStorageSize, unsigned long seed)
        : ExecutorInterface(programs, storageManager, threadLocalStorageSize),
          m_randomGenerator(seed) {}

    bool execute() override;

    void writeState(std::ostream &outputStream) const override;
};

class InteractiveExecutor : public ExecutorInterface {
    bool executeThread() override;
    bool executeInternalMemoryUpdate() override;

public:
    InteractiveExecutor(const std::vector<program::Program> &programs,
                   const storage::StorageManagerPtr &storageManager,
                   size_t threadLocalStorageSize)
        : ExecutorInterface(programs, storageManager, threadLocalStorageSize) {}

    bool execute() override;

    void writeState(std::ostream &outputStream) const override;
};

} // namespace wmm
