//
// Created by veronika on 20.10.23.
//

#include "Parser.h"

namespace wmm {

static std::vector<std::string> parseTokens(std::istream &stream,
                                            ssize_t expectedMax = SIZE_MAX) {
    std::string token;
    std::vector<std::string> answer;
    while (answer.size() < expectedMax && stream >> token) {
        answer.push_back(token);
    }
    if (stream >> token) throw std::runtime_error("Too many tokens");
    return answer;
}

static size_t parseRegister(const std::string &string) {
    size_t res = -1;
    if (std::stringstream(string) >> res) { return res; }
    throw std::runtime_error("Couldn't parse register");
}

static size_t parseRegisterWithMemAddress(const std::string &string) {
    size_t res = -1;
    if (string.empty() || string.front() != '#' ||
        !(std::stringstream(string.substr(1, string.size() - 1)) >> res)) {
        throw std::runtime_error("Couldn't parse register");
    }
    return res;
}

static int32_t parseConstant(const std::string &string) {
    int32_t res = INT32_MIN;
    if (std::stringstream(string) >> res) { return res; }
    throw std::runtime_error("Couldn't parse constant value");
}

static MemoryAccessMode parseMemoryAccessMode(const std::string &string) {
    if (STRING_TO_MODE.count(string) == 0) {
        throw std::runtime_error("Couldn't parse memory access mode");
    }
    return STRING_TO_MODE.at(string);
}

static Label parseLabel(const std::string &string) {
    Label label = 0;
    if (std::stringstream(string) >> label) { return label; }
    throw std::runtime_error("Couldn't parse label");
}

static std::optional<Label> tryParseLabel(const std::string &string) {
    if (string.size() < 2 || string.back() != ':') { return {}; }
    return parseLabel(string.substr(0, string.size() - 1));
}

static BinaryOperation parseBinaryOperation(const std::string &string) {
    if (string.size() != 1 || CHAR_TO_BIN_OPERATION.count(string[0]) == 0) {
        throw std::runtime_error("Couldn't parse binary operation");
    }
    return CHAR_TO_BIN_OPERATION.at(string[0]);
}

InstructionPtr
Parser::parseStoreInRegister(const std::vector<std::string> &tokens) {
    if ((tokens.size() != 3 && tokens.size() != 5) || tokens[1] != "=") {
        throw std::runtime_error("Failed to parse");
    }

    size_t storeAddress = parseRegister(tokens[0]);
    switch (tokens.size()) {
        case 3: {
            int32_t constant = parseConstant(tokens[2]);
            return std::make_shared<StoreConstInRegister>(storeAddress,
                                                          constant);
        }
        case 5: {
            size_t leftRegister = parseRegister(tokens[2]);
            BinaryOperation operation = parseBinaryOperation(tokens[3]);
            size_t rightRegister = parseRegister(tokens[4]);
            return std::make_shared<StoreExprInRegister>(
                    storeAddress, leftRegister, operation, rightRegister);
        }
            //        default:
            //            throw std::runtime_error("Unreachable state");
    }
    throw std::runtime_error("Unreachable state");
}

std::tuple<std::optional<Label>, InstructionPtr>
Parser::parseLine(const std::string &line) {
    if (line.empty() || line.front() == '/') return {};
    std::stringstream ss(line);
    std::vector<std::string> tokens = parseTokens(ss, 6);
    if (tokens.empty()) return {};
    auto label = tryParseLabel(tokens[0]);
    InstructionPtr instruction;

    if (label) {
        if (tokens.size() == 1) return {label, nullptr};
        tokens = std::vector<std::string>(tokens.begin() + 1, tokens.end());
    }

    Command parsedCommand = (STRING_TO_COMMAND.count(tokens[0]) > 0)
                                    ? STRING_TO_COMMAND.at(tokens[0])
                                    : Command::Other;
    switch (parsedCommand) {
        case Command::Other:
            instruction = parseStoreInRegister(tokens);
            break;
        case Command::If: {
            if (tokens.size() != 4 || tokens[2] != "goto") {
                throw std::runtime_error("Couldn't parse if");
            }
            size_t conditionRegister = parseRegister(tokens[1]);
            size_t labelId = parseLabel(tokens[3]);
            instruction = std::make_shared<Goto>(conditionRegister, labelId);
            break;
        }
        case Command::Load: {
            if (tokens.size() != 4) {
                throw std::runtime_error("Couldn't parse load");
            }
            MemoryAccessMode mode = parseMemoryAccessMode(tokens[1]);
            size_t addressRegister = parseRegisterWithMemAddress(tokens[2]);
            size_t destinationRegister = parseRegister(tokens[3]);
            instruction = std::make_shared<Load>(mode, addressRegister,
                                                 destinationRegister);
            break;
        }
        case Command::Store: {
            if (tokens.size() != 4) {
                throw std::runtime_error("Couldn't parse store");
            }
            MemoryAccessMode mode = parseMemoryAccessMode(tokens[1]);
            size_t addressRegister = parseRegisterWithMemAddress(tokens[2]);
            size_t valueRegister = parseRegister(tokens[3]);
            instruction = std::make_shared<Store>(mode, addressRegister,
                                                  valueRegister);
            break;
        }
        case Command::CompareAndSwap: {
            if (tokens.size() != 5) {
                throw std::runtime_error("Couldn't parse cas");
            }
            MemoryAccessMode mode = parseMemoryAccessMode(tokens[1]);
            size_t addressRegister = parseRegisterWithMemAddress(tokens[2]);
            size_t expectedValRegister = parseRegister(tokens[3]);
            size_t newValRegister = parseRegister(tokens[4]);
            instruction = std::make_shared<CompareAndSwap>(
                    mode, addressRegister, expectedValRegister, newValRegister);
            break;
        }
        case Command::FetchAndIncrement: {
            if (tokens.size() != 4) {
                throw std::runtime_error("Couldn't parse fai");
            }
            MemoryAccessMode mode = parseMemoryAccessMode(tokens[1]);
            size_t addressRegister = parseRegisterWithMemAddress(tokens[2]);
            size_t incrementRegister = parseRegister(tokens[3]);
            instruction = std::make_shared<FetchAndIncrement>(
                    mode, addressRegister, incrementRegister);
            break;
        }
        case Command::Fence: {
            if (tokens.size() != 2) {
                throw std::runtime_error("Couldn't parse fence");
            }
            MemoryAccessMode mode = parseMemoryAccessMode(tokens[1]);
            instruction = std::make_shared<Fence>(mode);
            break;
        }
    }
    return {label, instruction};
}

Program Parser::parseFromStream(std::istream &stream) {
    std::string line;
    size_t linesRead = 0;
    std::vector<InstructionPtr> program;
    std::unordered_map<Label, size_t> labelMapping;
    while (std::getline(stream, line)) {
        ++linesRead;
        try {
            auto [label, instruction] = parseLine(line);
            if (label) {
                if (labelMapping.count(label.value()) > 0) {
                    throw std::runtime_error("Duplicate label `" +
                                             std::to_string(label.value()) +
                                             '`');
                }
                labelMapping[label.value()] = program.size();
            }
            program.push_back(instruction);
        } catch (const std::exception &e) {
            throw ParsingError(linesRead, e.what());
        }
    }
    return {std::move(program), std::move(labelMapping)};
}

Program Parser::parseFromString(const std::string &input) {
    std::stringstream stream(input);
    return parseFromStream(stream);
}

} // namespace wmm