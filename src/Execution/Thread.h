//
// Created by veronika on 21.10.23.
//

#pragma once

#include <memory>

#include "Program.h"
#include "Storage.h"
#include "StorageManager.h"

namespace wmm::execution {

class Thread {
    const program::Program m_program;
    storage::Storage m_localStorage;
    storage::StorageManagerPtr m_storageManager;
    size_t m_currentInstruction = 0;
public:
    const size_t id;

    Thread(program::Program program, storage::StorageManagerPtr storageManager, size_t threadId,
           size_t localStorageSize = 100)
        : m_program(std::move(program)), m_localStorage(localStorageSize),
          m_storageManager(std::move(storageManager)), id(threadId) {}

    bool evaluateInstruction();

    std::shared_ptr<program::Instruction> getCurrentInstruction() const;

    bool isFinished() const { return m_currentInstruction == m_program.size(); }

    const storage::Storage &getLocalStorage() const { return m_localStorage; };
};

} // namespace wmm
