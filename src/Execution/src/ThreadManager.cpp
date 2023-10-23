#include "ThreadManager.h"
#include <algorithm>

namespace wmm::execution {

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
    evaluateThreadLocalInstructions(threadId);
    bool returnValue = m_threads.at(threadId).evaluateInstruction();
    evaluateThreadLocalInstructions(threadId);
    return returnValue;
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
    for (const auto &thread: m_threads) {
        if (!thread.isFinished()) { unfinishedThreads.push_back(thread.id); }
    }
    return unfinishedThreads;
}

size_t ThreadManager::size() const { return m_threads.size(); }

std::shared_ptr<program::Instruction>
ThreadManager::getCurrentInstructionForThread(size_t threadId) const {
    return m_threads.at(threadId).getCurrentInstruction();
}

void ThreadManager::evaluateThreadLocalInstructions(size_t threadId) {
    while (auto instruction = m_threads.at(threadId).getCurrentInstruction()) {
        switch (instruction->action) {
            case program::InstructionAction::StoreConstInRegister:
            case program::InstructionAction::StoreExprInRegister:
            case program::InstructionAction::Goto:
                m_threads[threadId].evaluateInstruction();
                break;
            case program::InstructionAction::Load:
            case program::InstructionAction::Store:
            case program::InstructionAction::CompareAndSwap:
            case program::InstructionAction::FetchAndIncrement:
            case program::InstructionAction::Fence:
                return;
        }
    }
}

} // namespace wmm::execution