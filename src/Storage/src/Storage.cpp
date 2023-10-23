//
// Created by veronika on 20.10.23.
//

#include "../Storage.h"

namespace wmm::storage {

int32_t Storage::load(size_t address) const {
    return m_storage.at(address);
}

size_t Storage::size() const noexcept {
    return m_storage.size();
}

void Storage::store(size_t address, int32_t value) {
    m_storage.at(address) = value;
}

std::vector<int32_t> Storage::getStorage() const  {
    return m_storage;
}

std::ostream &operator<<(std::ostream &os, const Storage &storage) {
    bool isFirstIteration = true;
    for (auto elm: storage.m_storage) {
        if (!isFirstIteration) os << ' ';
        os << elm;
        isFirstIteration = false;
    }
    return os;
}

} // namespace wmm
