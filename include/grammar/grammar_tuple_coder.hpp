#pragma once

#include <algorithm>
#include <cmath>
#include <consts.hpp>
#include <cstdint>
#include <iostream>
#include <util/bit_input_stream.hpp>
#include <vector>
#include <word_packing.hpp>

namespace gracli {
struct GrammarTupleCoder {

    static auto decode(std::string file_path) -> std::vector<word_packing::PackedIntVector<uint64_t>> {
        std::ifstream in(file_path, std::ios::binary);
        BitIStream    br(std::move(in));

        uint32_t rule_count   = br.read_int<uint32_t>(32);
        uint32_t min_rule_len = br.read_int<uint32_t>(32);
        uint32_t max_rule_len = br.read_int<uint32_t>(32);

        std::vector<word_packing::PackedIntVector<uint64_t>> rules;
        rules.reserve(rule_count);

        for (uint32_t i = 0; i < rule_count; i++) {
            uint32_t rule_len = br.read_int<uint32_t>(32) + min_rule_len;

            word_packing::PackedIntVector<uint64_t> rule(0, 32);
            rule.reserve(rule_len);
            uint32_t max_symbol = 0;
            for (uint32_t j = 0; j < rule_len; j++) {
                bool is_nonterminal = br.read_bit();

                uint32_t symbol;
                if (is_nonterminal) {
                    symbol = br.read_int<uint32_t>(32) + RULE_OFFSET;
                } else {
                    symbol = br.read_int<uint32_t>(8);
                }
                max_symbol = std::max(symbol, max_symbol);
                rule.push_back(symbol);
            }
            rule.resize(rule.size(), std::ceil(std::log2(max_symbol) + 1));

            rules.emplace_back(std::move(rule));
        }
        rules.shrink_to_fit();

        return rules;
    }
};
} // namespace gracli
