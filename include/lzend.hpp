#pragma once

#include <cstdint>

namespace gracli::lzend {
#include <compute_lzend.hpp>

struct LzEnd {
    using Char       = char;
    using TextOffset = uint32_t;
    using Phrase     = lzend_phrase<Char, TextOffset, TextOffset>;
    using Parsing    = space_efficient_vector<Phrase>;

    Parsing parsing;

    LzEnd(std::string &file) {
        Parsing          parsing;
        std::ifstream    in(file, std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        std::string input = ss.str();
        auto        n     = input.length();
        compute_lzend<Char, TextOffset>(input.data(), n, &parsing);
    }



};

} // namespace gracli::lzend
