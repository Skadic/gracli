#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <istream>
namespace gracli {
class BitReader {
    uint8_t       bit_idx;
    uint8_t       buf;
    std::istream &is;
    bool          is_last;

    static constexpr uint16_t MASK = 0b1'0000'0000;

    inline bool read_next_from_stream() {
        if (is.read(reinterpret_cast<char *>(&buf), 1)) {
            // If there is another character, everything is fine
            char dummy;
            if (is.get(dummy)) {
                is.unget();
            } else {
                is_last = true;
            }
            return true;
        } else {
            is_last = true;
            buf     = 0;
            bit_idx = 8;
            return false;
        }
    }

  public:
    inline bool eof() const { return is_last && bit_idx >= 8; }

    inline BitReader(std::istream &is) : bit_idx{8}, buf{0}, is{is}, is_last{false} {
        // So that eof() will return true
        if (is.eof()) {
            is_last = true;
        }
    }

    inline void read_bit(bool &out) {
        if (bit_idx >= 8) {
            read_next_from_stream();
            bit_idx = 0;
        }
        auto res = (MASK >> (bit_idx + 1)) & buf;
        bit_idx++;
        out = res != 0;
    }

    inline uint8_t read8(uint8_t &out) {
        if (bit_idx == 0) {
            bit_idx = 8;
            out     = buf;
            return 8;
        } else if (is_last) {
            auto res = ((MASK >> (bit_idx - 1)) - 1) & buf;
            auto cnt = 8 - bit_idx;
            bit_idx  = 8;
            out      = res;
            return cnt;
        } else {
            uint8_t msk    = (MASK >> bit_idx) - 1; // 00111111
            uint8_t res_hi = msk & buf;
            read_next_from_stream();
            uint8_t res_lo = (~msk) & buf;
            out            = (res_hi << bit_idx) | (res_lo >> (8 - bit_idx));
            return 8;
        }
    }

    inline uint8_t read32(uint32_t &out) {
        static std::array<uint8_t, 4> values;
        uint8_t                       total_bits_read = 0;
        uint32_t                      res             = 0;

        uint8_t bits_read = 0;

        for (int i = 0; i < 4; i++) {
            // Make space for the new value
            res <<= bits_read;

            bits_read = read8(values[i]);
            total_bits_read += bits_read;
            res |= values[i];
        }

        out = res;

        return total_bits_read;
    }
};
} // namespace gracli
