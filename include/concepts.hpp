#pragma once

#include <concepts>
#include <cstddef>
#include <string>

namespace gracli {

template<typename T>
concept Substring = requires(T ds, char *ptr, size_t i) {
                        { ds.substr(ptr, i, i) } -> std::convertible_to<char *>;
                    };

template<typename T>
concept CharRandomAccess = requires(T ds, char *ptr, size_t i) {
                               { ds.at(i) } -> std::convertible_to<char>;
                           };

template<typename T>
concept FromFile = requires(T ds, const std::string &s) {
                       { T::from_file(s) } -> std::convertible_to<T>;
                   };

template<typename T>
concept SourceLength = requires(T ds) {
                           { ds.source_length() } -> std::convertible_to<size_t>;
                       };
template<typename T>
concept RandomAccess = CharRandomAccess<T> && Substring<T> && SourceLength<T>;
} // namespace gracli
