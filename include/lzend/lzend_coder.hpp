#pragma once

#include <string>
#include <utility>

#include <lzend/lzend.hpp>
#include <util/bit_input_stream.hpp>

namespace gracli::lz {

auto decode(const std::string &file_path) -> std::pair<LzEnd::Parsing, size_t> {
    std::ifstream in(file_path, std::ios::binary);
    BitIStream    br(std::move(in));

    const auto char_width = br.read_int<uint8_t>() + 1;
    const auto int_width  = br.read_int<uint8_t>() + 1;
    const auto int_bytes  = int_width / CHAR_BIT;

    // We want to skip the next 6 bytes since they are still part of the header but don't contain any information
    br.read_int<uint64_t>(6 * CHAR_BIT);

    size_t source_len = 0;

    LzEnd::Parsing parsing;

    while (!br.eof()) {
        const auto        c           = br.read_int<LzEnd::Char>(char_width);
        LzEnd::TextOffset prev_phrase = 0;

        // TODO For the time being, this is assuming that int_width is a multiple of 8, in order to
        //  implement little-endianness
        for (size_t i = 0; i < int_bytes; ++i) {
            const auto val = br.read_int<LzEnd::TextOffset>(CHAR_BIT);
            prev_phrase |= val << (i * CHAR_BIT);
        }
        LzEnd::TextOffset phrase_len = 0;
        for (size_t i = 0; i < int_bytes; ++i) {
            const auto val = br.read_int<LzEnd::TextOffset>(CHAR_BIT);
            phrase_len |= val << (i * CHAR_BIT);
        }

        // TODO Idk if that middle term is right. I would have imagined that this is always 0 of phrase_len is 1
        parsing.push_back({c, phrase_len > 1 ? prev_phrase : 0, phrase_len});
        source_len += phrase_len;
    }

    return std::make_pair(std::move(parsing), source_len);
}

auto LzEnd::from_file(const std::string &file) -> LzEnd {
    auto [parsing, source_len] = decode(file);
    return from_parsing(std::move(parsing), source_len);
}

}; // namespace gracli::lz
