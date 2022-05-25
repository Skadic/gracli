#pragma once

#include <concepts>
#include <limits>

namespace gracli {

template<std::integral T>
T invalid() {
    return std::numeric_limits<T>().max();
}

} // namespace gracli
