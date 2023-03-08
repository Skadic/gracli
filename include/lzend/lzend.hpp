#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>

#include <compute_lzend.hpp>
#include <sdsl/sd_vector.hpp>

namespace gracli::lz {
/**
 * @brief Random access implementation on an Lz-End parsing based on the paper "Self-Index Based on LZ77" by Kreft and
 * Navarro.
 *
 * The paper can be found at https://arxiv.org/abs/1101.4065
 */
class LzEnd {
  public:
    using Char       = uint8_t;
    using TextOffset = uint64_t;
    using Phrase     = lzend_phrase<Char, TextOffset, TextOffset>;
    using Parsing    = space_efficient_vector<Phrase>;

    using BitVec = sdsl::sd_vector<sdsl::bit_vector>;
    using Rank   = sdsl::rank_support_sd<1, sdsl::bit_vector>;
    using Select = sdsl::select_support_sd<1, sdsl::bit_vector>;

  private:
    /**
     * @brief Stores the last character of each phrase contiguously.
     * In the original paper, this is L.
     */
    std::vector<Char> m_last;

    /**
     * @brief For each text position, stores a 1 if the position is the last character of a phrase.
     * In the original paper, this is B.
     */
    BitVec m_last_pos;
    Rank   m_last_pos_r;
    Select m_last_pos_s;

    /**
     * @brief For each text position respectively contains a 1 for each source starting at this position, followed by a
     * 0.
     *
     * For empty sources (phrase of just 1 character) the sources are considered to start before the first text position
     * and therefore appended at the start
     *
     * In the original paper, this is S.
     */
    BitVec m_source_begin;
    Rank   m_source_begin_r;
    Select m_source_begin_s;

    /**
     * @brief Maps a phrase to the index of its source.
     *
     * If source_map[i] = j, then the source of phrase i is the one represented by the j-th 1 in m_source_begin.
     *
     * IMPORTANT: This must be accessed using `source_map_accessor`. Do not try to access this directly
     *
     * In the original paper, this is P.
     */
    std::vector<size_t> m_source_map;

    size_t m_source_length;
    size_t m_index_bits{};
    size_t m_phrase_bits{};

  public:
    [[nodiscard]] inline auto num_phrases() const -> size_t { return m_last.size(); }

  private:
    /**
     * @brief Inclusive rank on the `m_last_pos` bit vector.
     * @return The number of ones up to and including i
     */
    [[nodiscard]] inline auto rank1_last_pos(const size_t i) const -> size_t { return m_last_pos_r.rank(i + 1); }

    /**
     * @brief Inclusive rank on the `m_source_begin` bit vector.
     * @return The number of ones up to and including i
     */
    [[nodiscard]] inline auto rank1_source_begin(const size_t i) const -> size_t {
        return m_source_begin_r.rank(i + 1);
    }

    /**
     * @brief Select on the `m_last_pos` bit vector.
     * @return The index of the ith one.
     */
    [[nodiscard]] inline auto select1_last_pos(const size_t i) const -> size_t { return m_last_pos_s.select(i); }

    /**
     * @brief Select on the `m_source_begin` bit vector.
     * @return The index of the ith one.
     */
    [[nodiscard]] inline auto select1_source_begin(const size_t i) const -> size_t {
        return m_source_begin_s.select(i);
    }

    [[nodiscard]] inline auto source_map_accessor() const {
        return word_packing::accessor(const_cast<const size_t *>(m_source_map.data()), m_phrase_bits);
    }

    void build_aux_ds(Parsing &&parsing) {
        size_t n_phrases = parsing.size();
        size_t n         = m_source_length;
        // The number of bits required to index the input
        m_index_bits  = ceil(log2((double) n));
        m_phrase_bits = ceil(log2((double) n_phrases));

        m_last.reserve(n_phrases + 1);

        size_t current_index = 0;

        sdsl::sd_vector_builder svb(n, n_phrases);

        // Calculate all end positions of phrase_source_start and store them
        for (size_t i = 0; i < n_phrases; i++) {
            Phrase &f = parsing[i];
            m_last.push_back(f.m_char);
            current_index += f.m_len;
            svb.set(current_index - 1);
        }

        m_last_pos   = sdsl::sd_vector(svb);
        m_last_pos_r = Rank(&m_last_pos);
        m_last_pos_s = Select(&m_last_pos);

        // Calculate the start indices of their phrase_source_start' sources
        std::vector<size_t> phrase_buffer;
        phrase_buffer.resize(word_packing::num_packs_required<size_t>(n_phrases, m_index_bits));
        auto phrase_source_start = word_packing::accessor(phrase_buffer.data(), m_index_bits);

        for (size_t i = 0; i < n_phrases; i++) {
            Phrase &f = parsing[i];
            if (f.m_len == 1) {
                phrase_source_start[i] = 0;
                continue;
            }

            size_t src_end         = select1_last_pos(f.m_link + 1);
            size_t src_start       = src_end - f.m_len + 2;
            phrase_source_start[i] = src_start + 1;
        }

        // Drop the parsing. It is not needed anymore
        { auto drop = std::move(parsing); }

        // for now this is used as a buffer to keep track of the sources
        // sorted ascending by their source's start index in the text
        m_source_map.resize(n_phrases);
        std::iota(m_source_map.begin(), m_source_map.end(), 0);

        std::stable_sort(m_source_map.begin(), m_source_map.end(), [&](const int &l, const int &r) {
            auto lhs = phrase_source_start[l];
            auto rhs = phrase_source_start[r];
            return lhs < rhs;
        });

        // Calculate the amount of sources starting at each index

        // The index we are currently modifying
        size_t bit_index = 0;
        // The phrase we are currently handling
        size_t phrase_index = 0;
        // The start index of the current phrase's source
        size_t start = phrase_source_start[m_source_map[phrase_index]];

        svb = sdsl::sd_vector_builder(n + n_phrases, n_phrases);
        // We iterate through the phrase_source_start in order of their source's appearance in the text
        // For every phrase that starts at a certain index, we place a 1, then we place a 0 (with a no-op) and repeat
        for (size_t i = 0; i < n; i++) {
            while (i == start && phrase_index < n_phrases) {
                svb.set(bit_index++);
                phrase_index++;
                if (phrase_index < n_phrases) {
                    start = phrase_source_start[m_source_map[phrase_index]];
                }
            }
            bit_index++;
        }

        m_source_begin   = sdsl::sd_vector(svb);
        m_source_begin_r = Rank(&m_source_begin);
        m_source_begin_s = Select(&m_source_begin);

        // Calculate the actual source mapping
        // We first count how many sources start at each index in the text
        std::vector<TextOffset> current_source_count(n + 1);
        for (size_t i = 0; i < n_phrases; i++) {
            size_t src_start = phrase_source_start[i];
            current_source_count[src_start + 1]++;
        }
        // Prefix sum. This is basically equivalent to a prefix sum over S
        for (size_t i = 2; i < n + 1; ++i) {
            current_source_count[i] += current_source_count[i - 1] + 1;
            current_source_count[i - 1]++;
        }

        // We map the phrase_source_start in B to 1s in S
        size_t id = 0;
        for (size_t i = 0; i < n_phrases; i++) {
            size_t src_start = phrase_source_start[i];
            // For every index in the source text, there is a string 1^k0 in S, such that the number of 1s denotes the
            // number of sources starting at that index This is the index in S at which this 1^k0 is situated
            m_source_map[id++] = rank1_source_begin(current_source_count[src_start]++) - 1;
        }

        auto source_map_acc = word_packing::accessor(m_source_map.data(), m_phrase_bits);
        for (int k = 0; k < n_phrases; ++k) {
            source_map_acc[k] = m_source_map[k];
        }
        m_source_map.resize(word_packing::num_packs_required<size_t>(n_phrases, m_phrase_bits));
    }

    LzEnd() :
        m_last{},
        m_last_pos{},
        m_source_begin{},
        m_source_map{},
        m_source_length{0},
        m_index_bits{0},
        m_phrase_bits{0} {}

  public:
    LzEnd(LzEnd &&other) noexcept :
        m_last{std::move(other.m_last)},
        m_last_pos{std::move(other.m_last_pos)},
        m_source_begin{std::move(other.m_source_begin)},
        m_source_map{std::move(other.m_source_map)},
        m_source_length{other.m_source_length},
        m_last_pos_r{&m_last_pos},
        m_last_pos_s{&m_last_pos},
        m_source_begin_r{&m_source_begin},
        m_source_begin_s{&m_source_begin},
        m_index_bits{other.m_index_bits},
        m_phrase_bits{other.m_phrase_bits} {
        other.m_last_pos_r     = Rank(nullptr);
        other.m_last_pos_s     = Select(nullptr);
        other.m_source_begin_r = Rank(nullptr);
        other.m_source_begin_s = Select(nullptr);
    }

    static LzEnd from_file(const std::string &file);

    static LzEnd from_string(const std::string &str) {
        std::istringstream in(str);
        return from_stream(in);
    }

    static LzEnd from_stream(std::istream &stream) {
        std::noskipws(stream);
        std::vector<Char> input((std::istream_iterator<Char>(stream)), std::istream_iterator<Char>());

        Parsing parsing;
        compute_lzend<Char, TextOffset>(input.data(), input.size(), &parsing);
        return from_parsing(std::move(parsing), input.size());
    }

    static LzEnd from_parsing(Parsing &&parsing, size_t source_length) {
        LzEnd instance;
        instance.m_source_length = source_length;
        instance.build_aux_ds(std::move(parsing));
        return instance;
    }

    [[nodiscard]] auto at(size_t i) const -> char {
        size_t phrase_id  = m_last_pos_r.rank(i);
        auto   source_map = source_map_accessor();

        while (!m_last_pos[i]) {
            // Find the source_phrase of this phrase
            size_t source_phrase = source_map[phrase_id];
            size_t phrase_start  = phrase_id > 0 ? select1_last_pos(phrase_id) + 1 : 0;

            // We move to the source since this is where we need to read from
            size_t new_i = select1_source_begin(source_phrase + 1) - source_phrase - 1;
            // If the char isn't at the beginning of a phrase we need to add the offset to it
            new_i += i - phrase_start;

            i = new_i;
            // Find the new i's phrase
            phrase_id = m_last_pos_r.rank(i);
        }
        return (char) m_last[phrase_id];
    }

  private:
    [[nodiscard("internal substring method should adjust buffer pointer")]] auto
    substr_internal(char *buf, const size_t substr_start, const size_t substr_len) const -> char * {
        if (substr_len == 0) {
            return buf;
        }

        const size_t end_incl     = substr_start + substr_len - 1;
        size_t       start_phrase = m_last_pos_r.rank(substr_start);
        size_t       end_phrase   = m_last_pos_r.rank(end_incl);

        auto source_map = source_map_accessor();

        if (start_phrase == end_phrase) {
            // Find the source of this phrase
            size_t source       = source_map[start_phrase];
            size_t start        = select1_source_begin(source + 1) - source - 1;
            size_t phrase_start = start_phrase > 0 ? select1_last_pos(start_phrase) + 1 : 0;
            size_t phrase_end   = select1_last_pos(start_phrase + 1);

            // If the pattern doesn't start at the beginning of a phrase we need to add the offset to it
            start += substr_start - phrase_start;

            // If the substring ends before the phrase then we need to extract the entire thing from the sources alone
            if (substr_start + substr_len - 1 < phrase_end) {
                buf = substr_internal(buf, start, substr_len);
            } else {
                // In the other case, we can extract the last character directly and only have to get the rest from the
                // sources
                buf    = substr_internal(buf, start, substr_len - 1);
                *buf++ = *reinterpret_cast<const char *>(&m_last[start_phrase]);
            }
            return buf;
        }

        for (size_t i = start_phrase; i <= end_phrase; ++i) {
            size_t start, len, phrase_start, phrase_end;
            phrase_start = i > 0 ? select1_last_pos(i) + 1 : 0;
            phrase_end   = select1_last_pos(i + 1);

            // We find the start index of the source in the text
            start = select1_source_begin(source_map[i] + 1) - source_map[i] - 1;

            // We calculate the amount of characters we need to read from the source
            if (i == start_phrase) {
                // If this is the start phrase it might be that the substring starts inside a phrase.
                // In that case, we need to add the offset inside the phrase to our source start position
                start += substr_start - phrase_start;
                len = select1_last_pos(start_phrase + 1) - substr_start + 1;
            } else if (i == end_phrase) {
                len = end_incl - select1_last_pos(end_phrase);
            } else {
                len = select1_last_pos(i + 1) - select1_last_pos(i);
            }

            // If this is the end phrase and the substring ends before the end of the phrase
            // we need to extract the entire thing from sources alone
            if (i == end_phrase && phrase_start + len - 1 < phrase_end) {
                buf = substr_internal(buf, start, len);
            } else {
                // Otherwise we can get the last character of the phrase from the array and only the rest from the
                // sources
                buf    = substr_internal(buf, start, len - 1);
                *buf++ = *reinterpret_cast<const char *>(&m_last[i]);
            }
        }
        return buf;
    }

  public:
    inline auto substr(char *buf, const size_t substr_start, const size_t substr_len) const -> char * {
        return substr_internal(buf, substr_start, std::min(substr_len, source_length() - substr_start));
    }

    [[nodiscard]] inline auto source_length() const -> size_t { return m_source_length; }
};

} // namespace gracli::lz

#include "lzend_coder.hpp"
