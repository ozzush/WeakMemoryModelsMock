//
// Created by veronika on 06.12.23.
//

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <sstream>

namespace wmm::util {
template<class T>
std::string join(const std::vector<T> &data,
                 std::function<std::string(T)> toString,
                 const std::string &separator = " ") {
    std::stringstream stream;
    bool isFirstIteration;
    for (const auto &elm: data) {
        if (!isFirstIteration) { stream << separator; }
        stream << toString(elm);
    }
    return stream.str();
}

template<class T>
std::string join(const std::vector<T> &data,
                 const std::string &separator = " ") {
    std::stringstream stream;
    bool isFirstIteration;
    for (const auto &elm: data) {
        if (!isFirstIteration) { stream << separator; }
        stream << elm;
    }
    return stream.str();
}
} // namespace