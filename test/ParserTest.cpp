#include "Parser.h"
#include "doctest.h"

using namespace wmm;

TEST_SUITE("Parser") {
    TEST_CASE("Label") {
        auto [label, instruction] = Parser::parseLine("1:");
        CHECK_EQ(label.value(), 1);
        CHECK_EQ(instruction, nullptr);
    }
    TEST_CASE("Store constant") {
        auto [label, instruction] = Parser::parseLine("1 = 2");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreConstInRegister>(instruction);
        CHECK_EQ(realInstruction.storeRegister, 1);
        CHECK_EQ(realInstruction.value, 2);
    }
    TEST_CASE("Expression") {
        auto [label, instruction] = Parser::parseLine("1 = 2 + 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreExprInRegister>(instruction);
        CHECK_EQ(realInstruction.storeRegister, 1);
        CHECK_EQ(realInstruction.leftRegister, 2);
        CHECK_EQ(realInstruction.operation, BinaryOperation::Addition);
        CHECK_EQ(realInstruction.rightRegister, 3);
    }
    TEST_CASE("If") {
        auto [label, instruction] = Parser::parseLine("if 1 goto 2");
        auto realInstruction = *std::dynamic_pointer_cast<Goto>(instruction);
        CHECK_EQ(realInstruction.conditionRegister, 1);
        CHECK_EQ(realInstruction.label, 2);
    }
    TEST_CASE("Load") {
        auto [label, instruction] = Parser::parseLine("load SEQ_CST #1 2");
        auto realInstruction = *std::dynamic_pointer_cast<Load>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::SequentialConsistency);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.resultRegister, 2);
    }
    TEST_CASE("Store") {
        auto [label, instruction] = Parser::parseLine("store REL #1 2");
        auto realInstruction = *std::dynamic_pointer_cast<Store>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::Release);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.valueRegister, 2);
    }
    TEST_CASE("CompareAndSwap") {
        auto [label, instruction] = Parser::parseLine("cas ACQ #1 2 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<CompareAndSwap>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::Acquire);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.expectedValueRegister, 2);
        CHECK_EQ(realInstruction.newValueRegister, 3);
    }
    TEST_CASE("FetchAndIncrement") {
        auto [label, instruction] = Parser::parseLine("fai REL_ACQ #1 2");
        auto realInstruction =
                *std::dynamic_pointer_cast<FetchAndIncrement>(instruction);
        CHECK_EQ(realInstruction.mode, MemoryAccessMode::ReleaseAcquire);
        CHECK_EQ(realInstruction.addressRegister, 1);
        CHECK_EQ(realInstruction.incrementRegister, 2);
    }
    TEST_CASE("Fence") {
        auto [label, instruction] = Parser::parseLine("fence RLX");
        auto realInstruction = *std::dynamic_pointer_cast<Fence>(instruction);
        CHECK_EQ(realInstruction.memoryAccessMode, MemoryAccessMode::Relaxed);
    }
}

TEST_SUITE("Expression") {
    TEST_CASE("Addition") {
        auto [label, instruction] = Parser::parseLine("1 = 2 + 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreExprInRegister>(instruction);
        CHECK_EQ(realInstruction.operation, BinaryOperation::Addition);
    }
    TEST_CASE("Subtraction") {
        auto [label, instruction] = Parser::parseLine("1 = 2 - 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreExprInRegister>(instruction);
        CHECK_EQ(realInstruction.operation, BinaryOperation::Subtraction);
    }
    TEST_CASE("Multiplication") {
        auto [label, instruction] = Parser::parseLine("1 = 2 * 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreExprInRegister>(instruction);
        CHECK_EQ(realInstruction.operation, BinaryOperation::Multiplication);
    }
    TEST_CASE("Division") {
        auto [label, instruction] = Parser::parseLine("1 = 2 / 3");
        auto realInstruction =
                *std::dynamic_pointer_cast<StoreExprInRegister>(instruction);
        CHECK_EQ(realInstruction.operation, BinaryOperation::Division);
    }
    TEST_CASE("Bad operation") {
        CHECK_THROWS_WITH(Parser::parseLine("1 = 2 -5 6"),
                          "Couldn't parse binary operation");
    }
}