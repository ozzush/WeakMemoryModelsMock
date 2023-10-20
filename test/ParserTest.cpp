#include "Parser.h"
#include "doctest.h"

using namespace wmm;

TEST_CASE("Parser") {
    Parser parser;
    SUBCASE("Label") {
        auto [label, instruction] = parser.parseLine("1:");
        CHECK_EQ(label.value(), 1);
        CHECK_EQ(instruction, nullptr);
    }
    SUBCASE("Store constant") {
        auto [label, instruction] = parser.parseLine("1 = 2");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreConstInRegister>(instruction);
        CHECK_EQ(realInstruction.storeRegister, 1);
        CHECK_EQ(realInstruction.value, 2);
    }
    SUBCASE("Expression") {
        auto [label, instruction] = parser.parseLine("1 = 2 + 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreExprInRegister>(instruction);
        CHECK_EQ(realInstruction.storeRegister, 1);
        CHECK_EQ(realInstruction.leftRegister, 2);
        CHECK_EQ(realInstruction.operation, BinaryOperation::Addition);
        CHECK_EQ(realInstruction.rightRegister, 3);
    }
    SUBCASE("If") {
        auto [label, instruction] = parser.parseLine("if 1 goto 2");
        auto realInstruction = *std::dynamic_pointer_cast<Goto>(instruction);
        CHECK_EQ(realInstruction.conditionRegister, 1);
        CHECK_EQ(realInstruction.label, 2);
    }
    SUBCASE("Load") {
        auto [label, instruction] = parser.parseLine("load SEQ_CST 1 2");
        auto realInstruction = *std::dynamic_pointer_cast<Load>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::SequentialConsistency);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.resultRegister, 2);
    }
    SUBCASE("Store") {
        auto [label, instruction] = parser.parseLine("store REL 1 2");
        auto realInstruction = *std::dynamic_pointer_cast<Store>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::Release);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.valueRegister, 2);
    }
    SUBCASE("CompareAndSwap") {
        auto [label, instruction] = parser.parseLine("cas ACQ 1 2 3");
        auto realInstruction = *std::dynamic_pointer_cast<CompareAndSwap>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::Acquire);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.expectedValueRegister, 2);
        CHECK_EQ(realInstruction.newValueRegister, 3);
    }
    SUBCASE("FetchAndIncrement") {
        auto [label, instruction] = parser.parseLine("fai REL_ACQ 1 2");
        auto realInstruction = *std::dynamic_pointer_cast<FetchAndIncrement>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::ReleaseAcquire);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.incrementRegister, 2);
    }
    SUBCASE("Fence") {
        auto [label, instruction] = parser.parseLine("fence RLX");
        auto realInstruction = *std::dynamic_pointer_cast<Fence>(instruction);
        CHECK_EQ(realInstruction.memoryAccessMode, MemoryAccessMode::Relaxed);
    }
}