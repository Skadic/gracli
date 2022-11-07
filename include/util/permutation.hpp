#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <vector>

#include <bm64.h>
#include <word_packing.hpp>

namespace gracli {

template<std::unsigned_integral Pack = size_t>
class Permutation {

    using BitVec     = bm::bvector<>;
    using RankSelect = BitVec::rs_index_type;

    /**
     * The number of elements in this permutation
     */
    size_t m_size;
    /**
     * The word width with which the elements are stored
     */
    uint8_t m_word_width;
    /**
     * The number of elements of the packing type need to be in the permutation buffer
     */
    size_t m_num_packs;
    /**
     * The buffer containing the permutation. This may only be accessed using the given accessor method
     */
    std::vector<Pack> m_permutation_buf;
    /**
     * A bitvector containing a 1 at each index where there is a shortcut in a cycle of the permutation
     */
    BitVec m_shortcut_pos;
    /**
     * The spacing of the permutation's shortcuts. On average, there will be a shortcut every "m_shortcut_spacing"
     * elements
     */
    size_t                      m_shortcut_spacing;
    std::unique_ptr<RankSelect> m_shortcut_pos_rs;
    /**
     * The buffer containing the shortcuts. This may only be accessed using the given accessor method
     */
    std::vector<Pack> m_shortcut_buf;

    inline auto permutation_accessor() const {
        return word_packing::accessor(const_cast<Pack const *>(m_permutation_buf.data()), m_word_width);
    }

    inline auto shortcut_accessor() const {
        return word_packing::accessor(const_cast<Pack const *>(m_shortcut_buf.data()), m_word_width);
    }

  public:
    /**
     * Constructs the permutation datastructure from a sized range. Note, that this range must contain all integers
     * between 0 (inclusive) and n (exclusive) where n is the number of elements in the range.
     *
     * @tparam Iter Any container that supports sized range. Make sure that iterating over this type of range produces
     * the elements in the order you want!
     * @param iter A sized range containing the values for the permutation
     */
    template<std::ranges::sized_range Iter>
    inline void construct(Iter &iter) {
        m_size       = std::ranges::size(iter);
        m_word_width = (uint8_t) std::lround(std::ceil(std::log2(m_size)));
        m_num_packs  = word_packing::num_packs_required<Pack>(m_size, m_word_width);
        m_permutation_buf.resize(m_num_packs);
        m_shortcut_pos.resize(m_size);
        m_shortcut_spacing = std::max(m_word_width, (uint8_t) 1);

        // Add the values from the range into the permutation
        auto perm = word_packing::accessor(m_permutation_buf.data(), m_word_width);
        {
            size_t i = 0;
            for (size_t v : iter) {
                perm[i++] = v;
            }
        }

        // The values that have been processed already
        std::vector<bool> processed(m_size);
        // Where the shortcut at index i will go
        std::vector<size_t> shortcut_dest_raw(m_size);
        // The current spacing to the last shortcut
        size_t space = 0;
        // The value where the last shortcut was
        size_t last_shortcut = 0;

        for (size_t i = 0; i < m_size; i++) {
            // This element was processed before
            if (processed[i]) {
                continue;
            }

            // We have found an unprocessed cycle
            last_shortcut      = perm[i];
            space              = 1;
            size_t current     = perm[perm[i]];
            processed[current] = true;

            // While we aren't back at the start of the cycle, move forwards m_shortcut_spacing times and set a new
            // shortcut
            while (current != perm[i]) {
                if (space == m_shortcut_spacing) {
                    // The spacing to the last shortcut is high enough to set a new one
                    shortcut_dest_raw[current] = last_shortcut;
                    m_shortcut_pos[current]    = true;
                    last_shortcut              = current;
                    space                      = 0;
                }
                // Move 1 step further through the cycle
                space++;
                current            = perm[current];
                processed[current] = true;
            }

            // But the final shortcut to complete the "shortcut cycle"
            shortcut_dest_raw[current] = last_shortcut;
            m_shortcut_pos[current]    = true;
            // Done with this cycle
        }

        m_shortcut_pos.freeze();
        m_shortcut_pos.build_rs_index(m_shortcut_pos_rs.get());

        const size_t shortcut_count = m_shortcut_pos.count();

        // There are going to be a lot of 0s in the shortcut vec, so we store them contiguously
        // and find out which index they originally come from by using the bitvector m_shortcut_pos
        m_shortcut_buf.resize(word_packing::num_packs_required<Pack>(shortcut_count, m_word_width));
        auto              shortcuts = word_packing::accessor<Pack>(m_shortcut_buf.data(), m_word_width);
        BitVec::size_type shortcut_pos;
        for (size_t i = 1; i < shortcut_count + 1; ++i) {
            m_shortcut_pos.select(i, shortcut_pos, *m_shortcut_pos_rs);
            shortcuts[i - 1] = shortcut_dest_raw[shortcut_pos];
        }
    }

    Permutation() :
        m_size{0},
        m_word_width{0},
        m_num_packs{0},
        m_permutation_buf{},
        m_shortcut_pos{},
        m_shortcut_pos_rs{new RankSelect()},
        m_shortcut_buf{} {}

    auto next(const size_t i) const -> size_t {
        auto acc = permutation_accessor();
        return acc[i];
    }

    auto previous(const size_t i) const -> size_t {
        auto perm      = permutation_accessor();
        auto shortcuts = shortcut_accessor();
        // Find next shortcut
        auto current_pos = i;
        // While there is no shortcut at the current position step forward.
        while (!m_shortcut_pos[current_pos]) {
            current_pos = perm[current_pos];
        }
        // Take the shortcut
        current_pos = shortcuts[m_shortcut_pos.rank(current_pos, *m_shortcut_pos_rs) - 1];
        // Go forward until the next value is value at index i
        while (perm[current_pos] != i) {
            current_pos = perm[current_pos];
        }
        return current_pos;
    }

    auto size() const -> size_t { return m_size; }
};

} // namespace gracli
