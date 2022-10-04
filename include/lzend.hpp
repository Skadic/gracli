#pragma once

#include <bm64.h>
#include <compute_lzend.hpp>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <util/permutation.hpp>

namespace gracli::lz {

class LzEnd {
    using Char       = uint8_t;
    using TextOffset = uint32_t;
    using Phrase     = lzend_phrase<Char, TextOffset, TextOffset>;
    using Parsing    = space_efficient_vector<Phrase>;

    using BitVec     = bm::bvector<>;
    using RankSelect = BitVec::rs_index_type;

    Parsing m_parsing;

    /**
     * @brief Stores the last character of each phrase contiguously.
     * In the original paper, this is L.
     */
    std::vector<Char> m_last;

    /**
     * @brief For each text position, stores a 1 if the position is the last character of a phrase.
     * In the original paper, this is B.
     */
    BitVec                      m_last_pos;
    std::unique_ptr<RankSelect> m_last_pos_rs;

    /**
     * @brief For each text position respectively contains a 1 for each source starting at this position, followed by a
     * 0.
     *
     * For empty sources (phrase of just 1 character) the sources are considered to start before the first text position
     * and therefore appended at the start
     *
     * In the original paper, this is S.
     */
    BitVec                      m_source_begin;
    std::unique_ptr<RankSelect> m_source_begin_rs;

    /**
     * @brief Maps a phrase to the index of its source.
     *
     * If m_source_map[i] = j, then the source of phrase i is the one represented by the j-th 1 in m_source_begin.
     *
     * In the original paper, this is P.
     */
    Permutation<> m_source_map;

    size_t m_source_length;

  public:
    size_t num_phrases() const { return m_last.size(); }

  private:
    size_t rank1_last_pos(const size_t i) const { return m_last_pos.rank(i, *m_last_pos_rs); }

    size_t rank1_source_begin(const size_t i) const { return m_source_begin.rank(i, *m_source_begin_rs); }

    size_t select1_last_pos(const size_t i) const {
        BitVec::size_type pos;
        m_last_pos.select(i, pos, *m_last_pos_rs);
        return pos;
    }

    size_t select1_source_begin(const size_t i) const {
        BitVec::size_type pos;
        m_source_begin.select(i, pos, *m_source_begin_rs);
        return pos;
    }

    void build_aux_ds() {
        size_t n_phrases = m_parsing.size();
        size_t n         = m_source_length;

        m_last.reserve(n_phrases);
        m_last_pos.resize(n);
        m_source_begin.resize(n + n_phrases);

        size_t current_index = 0;

        // Calculate all end positions of phrases and store them
        for (int i = 0; i < n_phrases; i++) {
            Phrase &f = m_parsing[i];
            m_last.push_back(f.m_char);
            current_index += f.m_len;
            m_last_pos.set(current_index - 1);
        }

        m_last_pos.optimize();
        m_last_pos.freeze();
        m_last_pos.build_rs_index(m_last_pos_rs.get());

        // Calculate the pairs of phrases and the start indices of their sources
        std::vector<std::pair<size_t, Phrase &>> phrases;
        phrases.reserve(n_phrases);

        current_index = 0;
        for (int i = 0; i < n_phrases; i++) {
            Phrase &f = m_parsing[i];
            if (f.m_len == 1) {
                phrases.emplace_back(0, f);
                continue;
            }

            // The phrase's source
            Phrase &src = m_parsing[f.m_link];

            size_t src_end   = select1_last_pos(f.m_link + 1);
            size_t src_start = src_end - f.m_len + 2;
            phrases.emplace_back(src_start + 1, f);
        }

        // for now this is used as a buffer to keep track of the sources
        // sorted ascending by their source's start index in the text
        std::vector<TextOffset> source_map_raw(m_last.size());
        source_map_raw.resize(n_phrases);
        std::iota(source_map_raw.begin(), source_map_raw.end(), 0);

        std::stable_sort(source_map_raw.begin(), source_map_raw.end(), [&](const int &l, const int &r) {
            auto &lhs = phrases[l];
            auto &rhs = phrases[r];
            if (lhs.first != rhs.first) {
                return lhs.first < rhs.first;
            } else {
                return lhs.second.m_len < rhs.second.m_len;
            }
        });

        // Calculate the amount of sources starting at each index

        // The index we are currently modifying
        size_t bit_index = 0;
        // The phrase we are currently handling
        size_t phrase_index = 0;
        // The start index of the current phrase's source
        size_t start = phrases[source_map_raw[phrase_index]].first;

        // We iterate through the phrases in order of their source's appearance in the text
        // For every phrase that starts at a certain index, we place a 1, then we place a 0 (with a no-op) and repeat
        for (int i = 0; i < n; i++) {
            while (i == start && phrase_index < n_phrases) {
                m_source_begin.set_bit(bit_index++);
                phrase_index++;
                if (phrase_index < n_phrases) {
                    start = phrases[source_map_raw[phrase_index]].first;
                }
            }
            bit_index++;
        }

        m_source_begin.optimize();
        m_source_begin.freeze();
        m_source_begin.build_rs_index(m_source_begin_rs.get());

        // TODO Kinda janky implementation. Surely this can be done better

        // Calculate the actual source mapping
        // We first count how many sources start at each index in the text
        std::vector<TextOffset> current_source_count(n + 1);
        for (auto &[src_start, phrase] : phrases) {
            current_source_count[src_start + 1]++;
        }
        // Prefix sum. This is basically equivalent to a prefix sum over S
        for (int i = 2; i < n + 1; ++i) {
            current_source_count[i] += current_source_count[i - 1] + 1;
            current_source_count[i - 1]++;
        }

        // We map the phrases in B to 1s in S
        size_t id = 0;
        for (auto &[src_start, factor] : phrases) {
            // For every index in the source text, there is a string 1^k0 in S, such that the number of 1s denotes the
            // number of sources starting at that index This is the index in S at which this 1^k0 is situated
            source_map_raw[id++] = rank1_source_begin(current_source_count[src_start]++) - 1;
        }

        m_source_map.construct(source_map_raw);
    }

    LzEnd() :
        m_last{},
        m_last_pos{},
        m_last_pos_rs{new RankSelect()},
        m_source_begin{},
        m_source_begin_rs{new RankSelect()},
        m_source_map{},
        m_source_length{0} {}

  public:
    LzEnd(LzEnd &&other) noexcept :
        m_parsing{std::move(other.m_parsing)},
        m_last{std::move(other.m_last)},
        m_last_pos{std::move(other.m_last_pos)},
        m_last_pos_rs{std::move(other.m_last_pos_rs)},
        m_source_begin{std::move(other.m_source_begin)},
        m_source_begin_rs{std::move(other.m_source_begin_rs)},
        m_source_map{std::move(other.m_source_map)},
        m_source_length{other.m_source_length} {
        m_last_pos.build_rs_index(m_last_pos_rs.get());
        m_source_begin.build_rs_index(m_source_begin_rs.get());
    }

    static LzEnd from_file(const std::string &file) {
        std::ifstream in(file, std::ios::binary);
        return from_stream(in);
    }

    static LzEnd from_string(const std::string &str) {
        std::istringstream in(str);
        return from_stream(in);
    }

    static LzEnd from_stream(std::istream &stream) {
        LzEnd instance;
        std::noskipws(stream);
        std::vector<Char> input((std::istream_iterator<Char>(stream)), std::istream_iterator<Char>());
        instance.m_source_length = input.size();
        compute_lzend<Char, TextOffset>(input.data(), instance.m_source_length, &instance.m_parsing);
        instance.build_aux_ds();
        return instance;
    }

    char at(size_t i) const {
        char c;
        substr(&c, i, 1);
        return c;
    }

    char *substr(char *buf, const size_t substr_start, const size_t substr_len) const {
        if (substr_len == 0) {
            return buf;
        }

        const size_t end_incl = substr_start + substr_len - 1;
        size_t       start_phrase;
        if (substr_start > 0) {
            start_phrase = rank1_last_pos(substr_start - 1);
        } else {
            start_phrase = 0;
        }
        size_t end_phrase;
        if (end_incl > 0) {
            end_phrase = rank1_last_pos(end_incl - 1);
        } else {
            end_phrase = 0;
        }

        if (start_phrase == end_phrase) {
            // If the substring is only 1 character and that character is the last character of the phrase,
            // then we can just read it from the vector
            // if (substr_len == 1 && substr_start == select1_last_pos(start_phrase)) {
            //    *buf++ = *reinterpret_cast<char*>(&m_last[start_phrase]);
            //    return buf;
            //}
            // Find the source of this phrase
            size_t source       = m_source_map.next(start_phrase);
            size_t start        = select1_source_begin(source + 1) - source - 1;
            size_t phrase_start = start_phrase > 0 ? select1_last_pos(start_phrase) + 1 : 0;
            size_t phrase_end   = select1_last_pos(start_phrase + 1);

            // If the pattern doesn't start at the beginning of a phrase we need to add the offset to it
            start += substr_start - phrase_start;

            // If the substring ends before the phrase then we need to extract the entire thing from the sources alone
            if (substr_start + substr_len - 1 < phrase_end) {
                buf = substr(buf, start, substr_len);
            } else {
                // In the other case, we can extract the last character directly and only have to get the rest from the
                // sources
                buf    = substr(buf, start, substr_len - 1);
                *buf++ = *reinterpret_cast<const char *>(&m_last[start_phrase]);
            }
            return buf;
        }

        for (size_t i = start_phrase; i <= end_phrase; ++i) {
            size_t start, len, phrase_start, phrase_end;
            phrase_start = i > 0 ? select1_last_pos(i) + 1 : 0;
            phrase_end   = select1_last_pos(i + 1);

            // We find the start index of the source in the text
            start = select1_source_begin(m_source_map.next(i) + 1) - m_source_map.next(i) - 1;

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
                buf = substr(buf, start, len);
            } else {
                // Otherwise we can get the last character of the phrase from the array and only the rest from the
                // sources
                buf    = substr(buf, start, len - 1);
                *buf++ = *reinterpret_cast<const char *>(&m_last[i]);
            }
        }
        return buf;
    }

    size_t source_length() const { return m_source_length; }
};

} // namespace gracli::lz
