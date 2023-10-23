//
// Created by veronika on 22.10.23.
//

#include <iostream>
#include <sstream>

#include "Executor.h"

namespace wmm::execution {

bool RandomExecutor::executeThread() {
    auto unfinishedThreads = m_threadManager.unfinishedThreads();
    if (unfinishedThreads.empty()) { return false; }
    std::shuffle(unfinishedThreads.begin(), unfinishedThreads.end(),
                 m_randomGenerator);
    for (auto threadId: unfinishedThreads) {
        if (m_threadManager.evaluateThread(threadId)) { return true; }
    }
    throw std::runtime_error("All threads are blocked");
}

void RandomExecutor::writeState(std::ostream &outputStream) const {
    m_storageManager->writeStorage(outputStream);
    auto localStorages = m_threadManager.getThreadLocalStorages();
    outputStream << "Thread-local storages:\n";
    for (size_t i = 0; i < localStorages.size(); ++i) {
        outputStream << "t" << i << ": ";
        auto storage = localStorages[i].getStorage();
        for (auto elm : storage) {
            outputStream << elm << ' ';
        }
        outputStream << '\n';
    }
}

bool RandomExecutor::execute() {
    bool tryExecuteThreadFirst = std::uniform_int_distribution<size_t>(0, 1)(m_randomGenerator);
    if (tryExecuteThreadFirst) {
        return executeThread() || executeInternalMemoryUpdate();
    } else {
        return executeInternalMemoryUpdate() || executeThread();
    }
}

void InteractiveExecutor::writeState(std::ostream &outputStream) const {
    m_storageManager->writeStorage(outputStream);
    auto localStorages = m_threadManager.getThreadLocalStorages();
    outputStream << "Thread-local storages:\n";
    for (size_t i = 0; i < localStorages.size(); ++i) {
        outputStream << "t" << i << ": ";
        auto storage = localStorages[i].getStorage();
        for (auto elm : storage) {
            outputStream << elm << ' ';
        }
        outputStream << '\n';
    }
}

bool InteractiveExecutor::executeThread() {
    std::cout << "Choose thread to execute: " << '\n';
    bool allThreadsAreFinished = true;
    for (size_t threadId = 0; threadId < m_threadManager.size(); ++threadId) {
        auto instruction = m_threadManager.getCurrentInstructionForThread(threadId);
        if (!instruction) continue;
        allThreadsAreFinished = false;
        std::cout << threadId << ": " + m_threadManager.getCurrentInstructionForThread(threadId)->str() << '\n';
    }
    if (allThreadsAreFinished) {
        std::cout << "All threads have finished their execution\n";
        return false;
    }
    while (true) {
        size_t threadId;
        std::cout << "Enter thread id > ";
        std::cin >> threadId;
        if (m_threadManager.evaluateThread(threadId)) {
            return true;
        } else {
            std::cout << "This thread can't be executed. \n";
        }
    }
}

bool InteractiveExecutor::execute() {
    while (true) {
        std::cout << "Execute user THREAD or internal MEMORY update? [T/m] > ";
        char operation = 0;
        std::cin >> operation;
        if (operation == 0 || operation == 'T' || operation == 't') {
            return executeThread() || executeInternalMemoryUpdate();
        } else if (operation == 'M' || operation == 'm') {
            return executeInternalMemoryUpdate() || executeThread();
        }
    }

}

bool InteractiveExecutor::executeInternalMemoryUpdate() {
    bool returnValue = ExecutorInterface::executeInternalMemoryUpdate();
    if (!returnValue) {
        std::cout << "No internal memory updates could be performed. \n";
    }
    return returnValue;
}

} // namespace wmm