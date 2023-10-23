//
// Created by veronika on 23.10.23.
//

#pragma once

#include <string>

namespace wmm::storage {

enum class MemoryAccessMode {
    SequentialConsistency = 0,
    Release,
    Acquire,
    ReleaseAcquire,
    Relaxed
};

inline std::string toString(MemoryAccessMode memoryAccessMode) {
    switch (memoryAccessMode) {
        case MemoryAccessMode::SequentialConsistency:
            return "SEQ_CST";
        case MemoryAccessMode::Release:
            return "REL";
        case MemoryAccessMode::Acquire:
            return "ACQ";
        case MemoryAccessMode::ReleaseAcquire:
            return "REL_ACQ";
        case MemoryAccessMode::Relaxed:
            return "RLX";
    }
    return "";
}

}