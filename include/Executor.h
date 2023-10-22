//
// Created by veronika on 22.10.23.
//

#pragma once

#include "ThreadManager.h"
#include <algorithm>
#include <random>
namespace wmm {

class ExecutorInterface {
protected:
    ThreadManager m_threadManager;
    StorageManagerPtr m_storageManager;

public:
    ExecutorInterface(const std::vector<Program> &programs,
                      const StorageManagerPtr &storageManager,
                      size_t threadLocalStorageSize)
        : m_storageManager(storageManager),
          m_threadManager(programs, storageManager, threadLocalStorageSize) {}

    virtual bool executeThread() = 0;

    virtual void writeState(std::ostream &outputStream) const = 0;
};

class RandomExecutor : public ExecutorInterface {
    std::mt19937 m_randomGenerator;

public:
    RandomExecutor(const std::vector<Program> &programs,
                   const StorageManagerPtr &storageManager,
                   size_t threadLocalStorageSize, unsigned long seed)
        : ExecutorInterface(programs, storageManager, threadLocalStorageSize),
          m_randomGenerator(seed) {}

    bool executeThread() override;
    void writeState(std::ostream &outputStream) const override;
};

} // namespace wmm
