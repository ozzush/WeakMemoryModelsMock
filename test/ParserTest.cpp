#include "../src/Program/Parser.h"
#include "doctest.h"

using namespace wmm::program;

TEST_SUITE("Parser") {
    TEST_CASE("Label") {
        auto [label, instruction] = Parser::parseLine("1:");
        CHECK_EQ(label.value(), 1);
        CHECK_EQ(instruction, nullptr);
    }

    TEST_CASE("Parse command") {
        std::string command;
        SUBCASE("Store constant") { command = "1 = 2"; }
        SUBCASE("Expression") { command = "1 = 2 + 3"; }
        SUBCASE("If") { command = "if 1 goto 2"; }
        SUBCASE("Load") { command = "load SEQ_CST #1 2"; }
        SUBCASE("Store") { command = "store REL #1 2"; }
        SUBCASE("CompareAndSwap") { command = "cas ACQ #1 2 3"; }
        SUBCASE("FetchAndIncrement") { command = "fei REL_ACQ #1 2"; }
        SUBCASE("Fence") { command = "fence RLX"; }
        auto [label, instruction] = Parser::parseLine(command);
        CHECK_EQ(command, instruction->toString());
    }

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