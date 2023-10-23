#include <algorithm>
#include <sstream>

#include "Instructions.h"
#include "Parser.h"

namespace wmm::program {

std::string toString(MemoryAccessMode mode) {
    return std::find_if(STRING_TO_MODE.begin(), STRING_TO_MODE.end(),
                        [mode](const auto &element) {
                            return element.second == mode;
                        })
            ->first;
}

std::string StoreConstInRegister::str() const {
    std::stringstream output;
    output << storeRegister << " = " << value;
    return output.str();
}

std::string StoreExprInRegister::str() const {
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

std::string Goto::str() const {
    std::stringstream output;
    output << "if " << conditionRegister << " goto " << label;
    return output.str();
}

std::string Load::str() const {
    std::stringstream output;
    std::string modeString = program::toString(mode);
    output << "load " + modeString + " #" << addressRegister << " "
           << resultRegister;
    return output.str();
}

std::string Store::str() const {
    std::stringstream output;
    std::string modeString = program::toString(mode);
    output << "store " + modeString + " #" << addressRegister << " "
           << valueRegister;
    return output.str();
}

std::string CompareAndSwap::str() const {
    std::stringstream output;
    std::string modeString = program::toString(mode);
    output << "cas " + modeString + " #" << addressRegister << " "
           << expectedValueRegister << " " << newValueRegister;
    return output.str();
}

std::string FetchAndIncrement::str() const {
    std::stringstream output;
    std::string modeString = program::toString(mode);
    output << "fei " + modeString + " #" << addressRegister << " "
           << incrementRegister;
    return output.str();
}

std::string Fence::str() const {
    std::stringstream output;
    std::string modeString = program::toString(memoryAccessMode);
    output << "fence " + modeString;
    return output.str();
}

} // namespace wmm::program