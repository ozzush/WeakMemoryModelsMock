//
// Created by veronika on 20.10.23.
//

#pragma once

#include "Instructions.h"
#include "Program.h"

#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

namespace wmm {

enum class Command {
    If,
    Load,
    Store,
    CompareAndSwap,
    FetchAndIncrement,
    Fence,
    Other
};

const std::map<std::string, Command> STRING_TO_COMMAND = {
        {"if", Command::If},
        {"load", Command::Load},
        {"store", Command::Store},
        {"cas", Command::CompareAndSwap},
        {"fei", Command::FetchAndIncrement},
        {"fence", Command::Fence},
};

const std::map<std::string, MemoryAccessMode> STRING_TO_MODE = {
        {"SEQ_CST", MemoryAccessMode::SequentialConsistency},
        {"REL", MemoryAccessMode::Release},
        {"ACQ", MemoryAccessMode::Acquire},
        {"REL_ACQ", MemoryAccessMode::ReleaseAcquire},
        {"RLX", MemoryAccessMode::Relaxed}};

const std::map<char, BinaryOperation> CHAR_TO_BIN_OPERATION = {
        {'+', BinaryOperation::Addition},
        {'-', BinaryOperation::Subtraction},
        {'*', BinaryOperation::Multiplication},
        {'/', BinaryOperation::Division}};

struct ParsingError : std::runtime_error {
    ParsingError(const std::string &message, size_t lineNumber,
                 const std::string &line)
        : std::runtime_error(message + " in line " +
                             std::to_string(lineNumber) + ':' + line) {}
    ParsingError(size_t lineNumber, const std::string &line)
        : std::runtime_error("Parsing error in line " +
                             std::to_string(lineNumber) + ':' + line) {}
};

class Parser {
    static InstructionPtr parseStoreInRegister(const std::vector<std::string> &tokens);

public:
    static std::tuple<std::optional<Label>, InstructionPtr> parseLine(const std::string &line);

    static Program parseFromStream(std::istream &stream);

    static Program parseFromString(const std::string &input);
};

} // namespace wmm
