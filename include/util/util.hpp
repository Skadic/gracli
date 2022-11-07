#pragma once

#include "permutation.hpp"
#include <bm64.h>
#include <concepts>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

namespace gracli {

template<std::integral T>
auto invalid() -> T {
    return std::numeric_limits<T>().max();
}

void print_bv(const bm::bvector<> &bv, const size_t grouping = 8) {
    size_t n = bv.size();
    for (size_t i = 1; i < bv.size() + 1; i++) {
        if ((i + (n % grouping) + 1) % grouping == 0) {
            std::cout << "_";
        }
        std::cout << bv[i - 1];
    }
    std::cout << std::endl;
}

template<typename R>
void print_range(const R &r) {
    std::cout << "[";
    bool is_first = true;
    for (auto v : r) {
        if (is_first) {
            is_first = false;
        } else {
            std::cout << ", ";
        }
        std::cout << v;
    }
    std::cout << "]" << std::endl;
}

void print_perm(const gracli::Permutation<> &p) {
    std::cout << "[";
    bool is_first = true;
    for (size_t i = 0; i < p.size(); i++) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << p.next(i);
    }
    std::cout << "]" << std::endl;
}

std::string read_to_string(const std::string &path) {
    std::ifstream ifs(path);
    std::noskipws(ifs);
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

} // namespace gracli
