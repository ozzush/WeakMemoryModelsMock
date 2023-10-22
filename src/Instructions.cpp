#include "Instructions.h"
#include "Parser.h"
#include <algorithm>
#include <sstream>

namespace wmm {

static std::string memoryAccessModeToString(MemoryAccessMode mode) {
    return std::find_if(STRING_TO_MODE.begin(), STRING_TO_MODE.end(),
                        [mode](const auto &element) {
                            return element.second == mode;
                        })
            ->first;
}

std::string StoreConstInRegister::toString() const {
    std::stringstream output;
    output << storeRegister << " = " << value;
    return output.str();
}

std::string StoreExprInRegister::toString() const {
    std::stringstream output;
    std::string binOperation;
    switch (operation) {
        case BinaryOperation::Addition:
            binOperation = "+";
            break;
        case BinaryOperation::Subtraction:
            binOperation = "-";
            break;
        case BinaryOperation::Multiplication:
            binOperation = "*";
            break;
        case BinaryOperation::Division:
            binOperation = "/";
            break;
    }
    output << storeRegister << " = " << leftRegister << ' ' + binOperation + ' '
           << rightRegister;
    return output.str();
}

std::string Goto::toString() const {
    std::stringstream output;
    output << "if " << conditionRegister << " goto " << label;
    return output.str();
}

std::string Load::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "load " + modeString + " #" << addressRegister << " "
           << resultRegister;
    return output.str();
}

std::string Store::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "store " + modeString + " #" << addressRegister << " "
           << valueRegister;
    return output.str();
}

std::string CompareAndSwap::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "cas " + modeString + " #" << addressRegister << " "
           << expectedValueRegister << " " << newValueRegister;
    return output.str();
}

std::string FetchAndIncrement::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "fei " + modeString + " #" << addressRegister << " "
           << incrementRegister;
    return output.str();
}

std::string Fence::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(memoryAccessMode);
    output << "fence " + modeString;
    return output.str();
}

} // namespace wmm