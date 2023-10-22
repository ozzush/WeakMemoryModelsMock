//
// Created by veronika on 22.10.23.
//

#include "Executor.h"
#include <sstream>

namespace wmm {

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

} // namespace wmm