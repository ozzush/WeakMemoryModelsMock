//
// Created by veronika on 21.10.23.
//

#include "Thread.h"

namespace wmm::execution {

using namespace program;

static int32_t applyBinaryOperation(BinaryOperation operation, int32_t lhs,
                                    int32_t rhs) {
    int32_t value = 0;
    switch (operation) {
        case BinaryOperation::Addition:
            value = lhs + rhs;
            break;
        case BinaryOperation::Subtraction:
            value = lhs - rhs;
            break;
        case BinaryOperation::Multiplication:
            value = lhs * rhs;
            break;
        case BinaryOperation::Division:
            value = lhs / rhs;
            break;
    }
    return value;
}

bool Thread::evaluateInstruction() {
    if (isFinished()) return false;
    auto instruction = m_program.getInstruction(m_currentInstruction);
    size_t nextInstruction = m_currentInstruction + 1;
    switch (instruction->action) {
        case InstructionAction::StoreConstInRegister: {
            auto cmd = *std::dynamic_pointer_cast<StoreConstInRegister>(
                    instruction);
            m_localStorage.store(cmd.storeRegister, cmd.value);
            break;
        }
        case InstructionAction::StoreExprInRegister: {
            auto cmd = *std::dynamic_pointer_cast<StoreExprInRegister>(
                    instruction);
            int32_t lhs = m_localStorage.load(cmd.leftRegister);
            int32_t rhs = m_localStorage.load(cmd.rightRegister);
            int32_t value = applyBinaryOperation(cmd.operation, lhs, rhs);
            m_localStorage.store(cmd.storeRegister, value);
            break;
        }
        case InstructionAction::Goto: {
            auto cmd = *std::dynamic_pointer_cast<Goto>(instruction);
            int32_t condition = m_localStorage.load(cmd.conditionRegister);
            if (condition) {
                nextInstruction = m_program.getLabelMapping(cmd.label);
            }
            break;
        }
        case InstructionAction::Load: {
            auto cmd = *std::dynamic_pointer_cast<Load>(instruction);
            size_t address = m_localStorage.load(cmd.addressRegister);
            int32_t value = m_storageManager->load(
                    id, address,
                    static_cast<storage::MemoryAccessMode>(cmd.mode));
            m_localStorage.store(cmd.resultRegister, value);
            break;
        }
        case InstructionAction::Store: {
            auto cmd = *std::dynamic_pointer_cast<Store>(instruction);
            size_t address = m_localStorage.load(cmd.addressRegister);
            int32_t value = m_localStorage.load(cmd.valueRegister);
            m_storageManager->store(
                    id, address, value,
                    static_cast<storage::MemoryAccessMode>(cmd.mode));
            break;
        }
        case InstructionAction::CompareAndSwap: {
            auto cmd = *std::dynamic_pointer_cast<CompareAndSwap>(instruction);
            size_t address = m_localStorage.load(cmd.addressRegister);
            int32_t expectedValue =
                    m_localStorage.load(cmd.expectedValueRegister);
            int32_t newValue = m_localStorage.load(cmd.newValueRegister);
            m_storageManager->compareAndSwap(
                    id, address, expectedValue, newValue,
                    static_cast<storage::MemoryAccessMode>(cmd.mode));
            break;
        }
        case InstructionAction::FetchAndIncrement: {
            auto cmd =
                    *std::dynamic_pointer_cast<FetchAndIncrement>(instruction);
            size_t address = m_localStorage.load(cmd.addressRegister);
            int32_t increment = m_localStorage.load(cmd.incrementRegister);
            m_storageManager->fetchAndIncrement(
                    id, address, increment,
                    static_cast<storage::MemoryAccessMode>(cmd.mode));
            break;
        }
        case InstructionAction::Fence: {
            auto cmd = *std::dynamic_pointer_cast<Fence>(instruction);
            m_storageManager->fence(id, static_cast<storage::MemoryAccessMode>(
                                                cmd.memoryAccessMode));
            break;
        }
    }
    m_currentInstruction = nextInstruction;
    return true;
}

} // namespace wmm::executor