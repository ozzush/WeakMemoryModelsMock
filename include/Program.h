#pragma once

#include "Instructions.h"
#include <memory>
#include <unordered_map>

namespace wmm {

using Label = size_t;

class Program {
    const std::vector<std::shared_ptr<Instruction>> m_program;
    const std::unordered_map<Label, size_t> m_labelMapping;

public:
    [[nodiscard]] std::shared_ptr<Instruction>
    getInstruction(size_t instruction) const;

    [[nodiscard]] size_t getLabelMapping(size_t label) const;

    [[nodiscard]] size_t size() const;

    Program(std::vector<std::shared_ptr<Instruction>> &&program,
            std::unordered_map<Label, size_t> &&labelMapping)
        : m_program(std::move(program)), m_labelMapping(std::move(labelMapping)) {
    }
};

} // namespace wmm
