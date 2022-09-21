#pragma once

#include <bm64.h>
#include <cstdint>
#include <numeric>
#include <util/permutation.hpp>
#include <compute_lzend.hpp>

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
     * @brief Stores the last character of each phrase contiguously
     */
    std::vector<Char> m_last;

    /**
     * @brief For each text position, stores a 1 if the position is the last character of a phrase.
     */
    BitVec                      m_last_pos;
    std::unique_ptr<RankSelect> m_last_pos_rs;

    /**
     * @brief For each text position respectively contains a 1 for each source starting at this position, followed by a
     * 0.
     *
     * For empty sources (phrase of just 1 character) the sources are considered to start before the first text position
     * and therefore appended at the start
     */
    BitVec                      m_source_begin;
    std::unique_ptr<RankSelect> m_source_begin_rs;


    // TODO This should be a permutation datastructure that allows to calculate P[i] and P^-1[i]
    /**
     * @brief Maps a phrase to the index of its source.
     *
     * If m_source_map[i] = j, then the source of phrase i is the one represented by the j-th 1 in m_source_begin.
     */
    Permutation<> m_source_map;

    size_t m_source_length;

  public:
    const size_t num_phrases() const { return m_last.size(); }

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
        //m_source_map.reserve(n_phrases);
        m_last_pos.resize(n);
        m_source_begin.resize(n + n_phrases);

        size_t current_index = 0;

        for (int i = 0; i < n_phrases; i++) {
            Phrase &f = m_parsing[i];
            m_last.push_back(f.m_char);
            current_index += f.m_len;
            m_last_pos.set(current_index - 1);
        }

        m_last_pos.optimize();
        m_last_pos.freeze();
        m_last_pos.build_rs_index(m_last_pos_rs.get());

        std::vector<std::pair<size_t, Phrase &>> sorted_phrases;
        sorted_phrases.reserve(n_phrases);

        current_index = 0;
        for (int i = 0; i < n_phrases; i++) {
            Phrase &f = m_parsing[i];
            if (f.m_len == 1) {
                sorted_phrases.emplace_back(0, f);
                continue;
            }

            Phrase &src = m_parsing[f.m_link];

            size_t src_end   = select1_last_pos(f.m_link + 1);
            size_t src_start = src_end - f.m_len + 2;
            sorted_phrases.emplace_back(src_start + 1, f);
        }

        std::vector<TextOffset> source_map_raw(m_last.size());
        source_map_raw.resize(n_phrases);
        std::iota(source_map_raw.begin(), source_map_raw.end(), 0);

        std::stable_sort(source_map_raw.begin(), source_map_raw.end(), [&](const int &l, const int &r) {
            auto &lhs = sorted_phrases[l];
            auto &rhs = sorted_phrases[r];
            if (lhs.first != rhs.first) {
                return lhs.first < rhs.first;
            } else {
                return lhs.second.m_len < rhs.second.m_len;
            }
        });

        current_index       = 0;
        size_t bit_index    = 0;
        size_t phrase_index = 0;
        size_t start        = sorted_phrases[source_map_raw[phrase_index]].first;
        for (int i = 0; i < n; i++) {
            while (i == start && phrase_index < n_phrases) {
                m_source_begin.set_bit(bit_index++);
                phrase_index++;
                start = sorted_phrases[source_map_raw[phrase_index]].first;
            }
            bit_index++;
        }

        m_source_map.construct(source_map_raw);

        m_source_begin.optimize();
        m_source_begin.freeze();
        m_source_begin.build_rs_index(m_source_begin_rs.get());
    }

  public:
    LzEnd(const std::string &file) :
        m_last{},
        m_last_pos{},
        m_last_pos_rs{new RankSelect()},
        m_source_begin{},
        m_source_begin_rs{new RankSelect()},
        m_source_map{},
        m_source_length{0} {
        std::ifstream     in(file, std::ios::binary);
        std::vector<Char> input((std::istream_iterator<Char>(in)), std::istream_iterator<Char>());
        m_source_length = input.size();
        compute_lzend<Char, TextOffset>(input.data(), m_source_length, &m_parsing);
        build_aux_ds();
        std::cout << "B: "; print_bv(m_last_pos);
        std::cout << "S: "; print_bv(m_source_begin);
        std::cout << "P: "; print_perm(m_source_map);
    }
  private:

  public:
    char *substr(char *buf, const size_t substr_start, const size_t substr_len) {
        if (substr_len == 0) {
            return buf;
        }

        const size_t end_incl     = substr_start + substr_len - 1;
        size_t       start_phrase;
        if(substr_start > 0) {
            start_phrase = rank1_last_pos(substr_start - 1);
        } else {
            start_phrase = 0;
        }
        size_t       end_phrase;
        if(end_incl > 0) {
            end_phrase = rank1_last_pos(end_incl - 1);
        } else {
            end_phrase = 0;
        }

        if (start_phrase == end_phrase) {
            // If the substring is only 1 character and that character is the last character of the phrase,
            // then we can just read it from the vector
            if (substr_len == 1) {
                *buf++ = *reinterpret_cast<char*>(&m_last[start_phrase]);
                return buf;
            }
            // Find the source of this phrase
            size_t source = m_source_map.previous(start_phrase);
            size_t source_start_index = select1_source_begin(source + 1) - source;
            buf = substr(buf, source_start_index, substr_len - 1);

            // If this condition is false, then the character to extract is not the last of the phrase and therefore we do not return it if this is false
            if (substr_len > 1) {
                *buf++ = *reinterpret_cast<char *>(&m_last[start_phrase]);
            }
            return buf;
        }

        for (size_t i = start_phrase; i <= end_phrase; ++i) {
            size_t start, len, phrase_start, phrase_end;
            phrase_start = i > 0 ? select1_last_pos(i) + 1 : 0;
            phrase_end = select1_last_pos(i + 1);

            start = select1_source_begin(m_source_map.next(i) + 1) - m_source_map.next(i);
            if(i == start_phrase) {
                start += substr_start - phrase_start;
                len = select1_last_pos(start_phrase + 1) - substr_start + 1;
            } else if (i == end_phrase) {
                len = end_incl - select1_last_pos(end_phrase);
            } else {
                len = select1_last_pos(i + 1) - select1_last_pos(i);
            }


            if(i == end_phrase && phrase_start + len - 1 < phrase_end) {
                buf = substr(buf, start, len);
            } else {
                buf = substr(buf, start, len - 1);
                *buf++ = *reinterpret_cast<char *>(&m_last[i]);
            }
        }
        return buf;
    }

    size_t size() const {
        return m_source_length;
    }
};

} // namespace gracli::lz
