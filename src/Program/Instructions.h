#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace wmm::program {

enum class MemoryAccessMode {
    SequentialConsistency = 0,
    Release,
    Acquire,
    ReleaseAcquire,
    Relaxed
};

std::string toString(MemoryAccessMode mode);

enum class InstructionAction {
    StoreConstInRegister,
    StoreExprInRegister,
    Goto,
    Load,
    Store,
    CompareAndSwap,
    FetchAndIncrement,
    Fence,
};

enum class BinaryOperation { Addition, Subtraction, Multiplication, Division };

struct Instruction;
using InstructionPtr = std::shared_ptr<Instruction>;

struct Instruction {
    const InstructionAction action;
    virtual ~Instruction() = default;

    virtual std::string toString() const = 0;
protected:

    explicit Instruction(InstructionAction action_) : action(action_) {}
};


#define InstructionImplementation1(InstructionName, argType, argName)          \
    struct InstructionName : Instruction {                                     \
        const argType argName;                                                 \
                                                                               \
        std::string toString() const override;                                 \
                                                                               \
        explicit InstructionName(argType argName_)                             \
            : Instruction(InstructionAction::InstructionName),                 \
              argName(argName_) {}                                             \
    }

#define InstructionImplementation2(InstructionName, argType1, argName1,        \
                                   argType2, argName2)                         \
    struct InstructionName : Instruction {                                     \
        const argType1 argName1;                                               \
        const argType2 argName2;                                               \
                                                                               \
        std::string toString() const override;                                 \
                                                                               \
        explicit InstructionName(argType1 argName1_, argType2 argName2_)       \
            : Instruction(InstructionAction::InstructionName),                 \
              argName1(argName1_), argName2(argName2_) {}                      \
    }

#define InstructionImplementation3(InstructionName, argType1, argName1,        \
                                   argType2, argName2, argType3, argName3)     \
    struct InstructionName : Instruction {                                     \
        const argType1 argName1;                                               \
        const argType2 argName2;                                               \
        const argType3 argName3;                                               \
                                                                               \
        std::string toString() const override;                                 \
                                                                               \
        explicit InstructionName(argType1 argName1_, argType2 argName2_,       \
                                 argType3 argName3_)                           \
            : Instruction(InstructionAction::InstructionName),                 \
              argName1(argName1_), argName2(argName2_), argName3(argName3_) {} \
    }

#define InstructionImplementation4(InstructionName, argType1, argName1,        \
                                   argType2, argName2, argType3, argName3,     \
                                   argType4, argName4)                         \
    struct InstructionName : Instruction {                                     \
        const argType1 argName1;                                               \
        const argType2 argName2;                                               \
        const argType3 argName3;                                               \
        const argType4 argName4;                                               \
                                                                               \
        std::string toString() const override;                                 \
                                                                               \
        explicit InstructionName(argType1 argName1_, argType2 argName2_,       \
                                 argType3 argName3_, argType4 argName4_)       \
            : Instruction(InstructionAction::InstructionName),                 \
              argName1(argName1_), argName2(argName2_), argName3(argName3_),   \
              argName4(argName4_) {}                                           \
    }

InstructionImplementation2(StoreConstInRegister, size_t, storeRegister, int32_t,
                           value);
InstructionImplementation4(StoreExprInRegister, size_t, storeRegister, size_t,
                           leftRegister, BinaryOperation, operation, size_t,
                           rightRegister);
InstructionImplementation2(Goto, size_t, conditionRegister, size_t, label);
InstructionImplementation3(Load, MemoryAccessMode, mode, size_t,
                           addressRegister, size_t, resultRegister);
InstructionImplementation3(Store, MemoryAccessMode, mode, size_t,
                           addressRegister, size_t, valueRegister);
InstructionImplementation4(CompareAndSwap, MemoryAccessMode, mode, size_t,
                           addressRegister, size_t, expectedValueRegister,
                           size_t, newValueRegister);
InstructionImplementation3(FetchAndIncrement, MemoryAccessMode, mode, size_t,
                           addressRegister, size_t, incrementRegister);
InstructionImplementation1(Fence, MemoryAccessMode, memoryAccessMode);

#undef InstructionImplementation1
#undef InstructionImplementation2
#undef InstructionImplementation3
#undef InstructionImplementation4

} // namespace wmm
