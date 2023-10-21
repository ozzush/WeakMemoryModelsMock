#include "Instructions.h"
#include "Parser.h"
#include <sstream>

namespace wmm {


//InstructionImplementation2(StoreConstInRegister, size_t, storeRegister, int32_t,
//                           value);

std::string StoreConstInRegister::toString() const {
    std::stringstream output;
    output << storeRegister << " = " << value;
    return output.str();
}

//InstructionImplementation4(StoreExprInRegister, size_t, storeRegister, size_t,
//                           leftRegister, BinaryOperation, operation, size_t,
//                           rightRegister);

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

//InstructionImplementation2(Goto, size_t, conditionRegister, size_t, label);

std::string Goto::toString() const {
    std::stringstream output;
    output << "if " << conditionRegister << " goto " << label;
    return output.str();
}

static std::string memoryAccessModeToString(MemoryAccessMode mode) {
    return std::find_if(STRING_TO_MODE.begin(), STRING_TO_MODE.end(),
                        [mode](const auto &element) {
                            return element.second == mode;
                        })
            ->first;
}

//InstructionImplementation3(Load, MemoryAccessMode, mode, size_t,
//                           addressRegister, size_t, resultRegister);

std::string Load::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "load " + modeString + " #" << addressRegister << " "
           << resultRegister;
    return output.str();
}

//InstructionImplementation3(Store, MemoryAccessMode, mode, size_t,
//                           addressRegister, size_t, valueRegister);

std::string Store::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "store " + modeString + " #" << addressRegister << " "
           << valueRegister;
    return output.str();
}

//InstructionImplementation4(CompareAndSwap, MemoryAccessMode, mode, size_t,
//                           addressRegister, size_t, expectedValueRegister,
//                           size_t, newValueRegister);

std::string CompareAndSwap::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "cas " + modeString + " #" << addressRegister << " "
           << expectedValueRegister << " " << newValueRegister;
    return output.str();
}

//InstructionImplementation3(FetchAndIncrement, MemoryAccessMode, mode, size_t,
//                           addressRegister, size_t, incrementRegister);

std::string FetchAndIncrement::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(mode);
    output << "fei " + modeString + " #" << addressRegister << " "
           << incrementRegister;
    return output.str();
}

//InstructionImplementation1(Fence, MemoryAccessMode, memoryAccessMode);

std::string Fence::toString() const {
    std::stringstream output;
    std::string modeString = memoryAccessModeToString(memoryAccessMode);
    output << "load " + modeString;
    return output.str();
}


} // namespace wmm