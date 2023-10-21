//
// Created by veronika on 20.10.23.
//

#include "Storage.h"
#include <stdexcept>

namespace wmm {

size_t Storage::load(size_t address) const {
    return storage.at(address);
}
size_t Storage::size() const noexcept {
    return storage.size();
}
void Storage::store(size_t address, int32_t value) {
    storage.at(address) = value;
}

} // namespace wmm
