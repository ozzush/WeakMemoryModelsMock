//
// Created by veronika on 20.10.23.
//

#include "Program.h"

namespace wmm {

std::shared_ptr<Instruction> Program::getInstruction(size_t instruction) const {
    return program.at(instruction);
}

size_t Program::getLabelMapping(size_t label) const {
    return labelMapping.at(label);
}

} // namespace wmm