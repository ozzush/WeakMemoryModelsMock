//
// Created by veronika on 20.10.23.
//

#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

namespace wmm::storage {

class Storage {
    std::vector<int32_t> m_storage;
public:
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] int32_t load(size_t address) const;
    void store(size_t address, int32_t value);

    explicit Storage(size_t size) : m_storage(size) {}

    [[nodiscard]] std::vector<int32_t> getStorage() const;
    [[nodiscard]] std::string str() const;
};

} // namespace wmm
