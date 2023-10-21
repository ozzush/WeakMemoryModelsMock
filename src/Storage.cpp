//
// Created by veronika on 20.10.23.
//

#include "Storage.h"

namespace wmm {

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

} // namespace wmm
