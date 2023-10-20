#pragma once

#include "Instructions.h"
#include <memory>
#include <unordered_map>

namespace wmm {

using Label = size_t;

class Program {
    const std::vector<std::shared_ptr<Instruction>> program;
    const std::unordered_map<Label, size_t> labelMapping;

public:
    [[nodiscard]] std::shared_ptr<Instruction>
    getInstruction(size_t instruction) const;

    [[nodiscard]] size_t getLabelMapping(size_t label) const;

    Program(std::vector<std::shared_ptr<Instruction>> &&program_,
            std::unordered_map<Label, size_t> &&labelMapping_)
        : program(std::move(program_)), labelMapping(std::move(labelMapping_)) {
    }
};

} // namespace wmm
