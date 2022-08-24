#pragma once

#include <bm64.h>
#include <cstdint>
#include <numeric>
namespace gracli::lz {
#include <compute_lzend.hpp>

class LzEnd {
    using Char       = uint8_t;
    using TextOffset = uint32_t;
    using Phrase     = lzend_phrase<Char, TextOffset, TextOffset>;
    using Parsing    = space_efficient_vector<Phrase>;

    using BitVec     = bm::bvector<>;
    using RankSelect = BitVec::rs_index_type;

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

    /**
     * @brief Maps a phrase to the index of its source.
     *
     * If m_source_map[i] = j, then the source of phrase i is the one represented by the j-th 1 in m_source_begin.
     */
    std::vector<TextOffset> m_source_map;

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

    void build_aux_ds(Parsing &parsing) {
        size_t n_phrases = parsing.size();
        size_t n         = m_source_length;

        m_last.reserve(n_phrases);
        m_source_map.reserve(n_phrases);

        size_t current_index = 0;

        for (int i = 0; i < n_phrases; i++) {
            Phrase &f = parsing[i];
            m_last.push_back(f.m_char);
            current_index += f.m_len;
            m_last_pos.set(current_index - 1);
        }

        m_last_pos.optimize();
        m_last_pos.freeze();
        m_last_pos.build_rs_index(m_last_pos_rs.get());

        std::cout << "B: ";
        for (size_t i = 0; i < n; i++) {
            std::cout << m_last_pos.get_bit(i);
        }
        std::cout << std::endl;

        std::vector<std::pair<size_t, Phrase &>> sorted_phrases;
        sorted_phrases.reserve(n_phrases);

        m_source_map.resize(n_phrases);
        std::iota(m_source_map.begin(), m_source_map.end(), 0);

        current_index = 0;
        for (int i = 0; i < n_phrases; i++) {
            Phrase &f = parsing[i];
            if (f.m_len == 1) {
                sorted_phrases.emplace_back(0, f);
                continue;
            }

            Phrase &src = parsing[f.m_link];

            size_t src_end   = select1_last_pos(f.m_link + 1);
            size_t src_start = src_end - f.m_len + 2;
            sorted_phrases.emplace_back(src_start, f);
        }

        std::stable_sort(m_source_map.begin(), m_source_map.end(), [&](const int &l, const int &r) {
            auto &lhs = sorted_phrases[l];
            auto &rhs = sorted_phrases[r];
            if (lhs.first != rhs.first) {
                return lhs.first < rhs.first;
            } else {
                return lhs.second.m_len < lhs.second.m_len;
            }
        });

        current_index       = 0;
        size_t bit_index    = 0;
        size_t phrase_index = 0;
        size_t start        = sorted_phrases[m_source_map[phrase_index]].first;
        for (int i = 0; i < n; i++) {
            while (i == start && phrase_index < n_phrases) {
                m_source_begin.set_bit(bit_index++);
                phrase_index++;
                start = sorted_phrases[m_source_map[phrase_index]].first;
            }
            bit_index++;
        }

        m_source_begin.optimize();
        m_source_begin.freeze();
        m_source_begin.build_rs_index(m_source_begin_rs.get());

        std::cout << "S: ";
        for (size_t i = 0; i < n + n_phrases; i++) {
            std::cout << m_source_begin.get_bit(i);
        }
        std::cout << std::endl;

        std::cout << "P: ";
        for (size_t i = 0; i < n_phrases; i++) {
            std::cout << m_source_map[i] << " ";
        }
        std::cout << std::endl;
    }

  public:
    LzEnd(std::string file) :
        m_last{},
        m_last_pos{},
        m_last_pos_rs{new RankSelect()},
        m_source_begin{},
        m_source_begin_rs{new RankSelect()},
        m_source_map{},
        m_source_length{0} {
        Parsing           parsing;
        std::ifstream     in(file, std::ios::binary);
        std::vector<Char> input((std::istream_iterator<Char>(in)), std::istream_iterator<Char>());
        m_source_length = input.size();
        compute_lzend<Char, TextOffset>(input.data(), m_source_length, &parsing);
        build_aux_ds(parsing);
        print();
    }

    void print() {
        for (size_t i = 0; i < num_phrases(); i++) {
            size_t remap = m_source_map[i];
            std::cout << remap << ": (" << m_last[remap] << ")" << std::endl;
        }

        // std::cout << m_last_pos << std::endl;
        // std::cout << m_source_begin << std::endl;
    }
};

} // namespace gracli::lz
