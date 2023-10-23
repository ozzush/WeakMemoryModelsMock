//
// Created by veronika on 20.10.23.
//

#include "Program.h"

namespace wmm::program {

std::shared_ptr<Instruction> Program::getInstruction(size_t instruction) const {
    if (instruction >= m_program.size()) return nullptr;
    return m_program[instruction];
}

size_t Program::getLabelMapping(size_t label) const {
    return m_labelMapping.at(label);
}

size_t Program::size() const { return m_program.size(); }

} // namespace wmm