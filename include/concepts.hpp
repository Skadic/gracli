#pragma once

#include <concepts>
#include <cstddef>
#include <string>

namespace gracli {

template<typename T>
concept Substring = requires(T gr, char *ptr, size_t i) {
                        { gr.substr(ptr, i, i) } -> std::convertible_to<char *>;
                    };

template<typename T>
concept CharRandomAccess = requires(T gr, char *ptr, size_t i) {
                               { gr.at(i) } -> std::convertible_to<char>;
                           };

} // namespace gracli
