//
// Created by veronika on 21.10.23.
//

#pragma once

#include "Program.h"
#include "Storage.h"
#include "StorageManager.h"
namespace wmm {

class Thread {
    Program m_program;
    Storage m_localStorage;
    StorageManagerPtr m_storageManager;
    size_t m_currentInstruction = 0;
    size_t m_id;

public:
    Thread(Program program, StorageManagerPtr storageManager, size_t threadId,
           size_t localStorageSize = 100)
        : m_program(std::move(program)), m_localStorage(localStorageSize),
          m_storageManager(std::move(storageManager)), m_id(threadId) {}

    bool evaluateInstruction();

    bool isFinished() const { return m_currentInstruction == m_program.size(); }

    Storage getLocalStorage() const { return m_localStorage; };
};

} // namespace wmm
