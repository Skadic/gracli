#pragma once

#include "../external/word-packing/include/word_packing.hpp"
#include "bit_input_stream.hpp"
#include "bit_reader.hpp"
#include "consts.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace gracli {
struct GrammarTupleCoder {
    //#define GTC1

    static std::vector<word_packing::PackedIntVector<uint64_t>> decode(std::string file_path) {
#ifdef GTC1
        std::ifstream in(file_path, std::ios::binary);
        BitReader     br(in);

        uint32_t rule_count;
        uint32_t min_rule_len;
        uint32_t max_rule_len;

        br.read32(rule_count);
        br.read32(min_rule_len);
        br.read32(max_rule_len);

        std::vector<word_packing::PackedIntVector<uint64_t>> rules;
        rules.reserve(rule_count);

        for (int i = 0; i < rule_count; i++) {
            uint32_t rule_len;
            br.read32(rule_len);
            rule_len += min_rule_len;

            word_packing::PackedIntVector<uint64_t> rule(0, 32);
            rule.reserve(rule_len);
            uint32_t max_symbol = 0;
            for (uint32_t j = 0; j < rule_len; j++) {
                bool is_nonterminal;
                br.read_bit(is_nonterminal);

                uint32_t symbol;
                if (is_nonterminal) {
                    br.read32(symbol);
                    symbol += RULE_OFFSET;
                } else {
                    uint8_t temp_symb;
                    br.read8(temp_symb);
                    symbol = temp_symb;
                }
                max_symbol = std::max(symbol, max_symbol);
                rule.push_back(symbol);
            }
            rule.resize(rule.size(), std::ceil(std::log2(max_symbol)));
            rules.emplace_back(std::move(rule));
        }
        rules.shrink_to_fit();
#else

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
#endif

        return rules;
    }
};
} // namespace gracli
