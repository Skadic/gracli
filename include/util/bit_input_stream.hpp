#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace gracli {

// Source: https://github.com/tudocomp/tudocomp/blob/grammar-comp/include/tudocomp/io/BitIStream.hpp

/// \brief Wrapper for input streams that provides bitwise reading
/// functionality.
///
/// The current byte in the underlying input stream is buffered and processed
/// bitwise using another cursor.
class BitIStream {
    std::istream &m_stream;

    static constexpr uint8_t MSB = 7;

    uint8_t m_current = 0;
    uint8_t m_next    = 0;

    bool    m_is_final   = false;
    uint8_t m_final_bits = 0;

    uint8_t m_cursor = 0;

    size_t m_bits_read = 0;

    inline void read_next_from_stream() {
        char c;
        if (m_stream.get(c)) {
            m_next = c;

            if (m_stream.get(c)) {
                // stream still going
                m_stream.unget();
            } else {
                // stream over after next, do some checks
                m_final_bits = c;
                m_final_bits &= 0b111;
                if (m_final_bits >= 6) {
                    // special case - already final
                    m_is_final = true;
                    m_next     = 0;
                }
            }
        } else {
            m_is_final   = true;
            m_final_bits = m_current & 0b111;

            m_next = 0;
        }
    }

    inline void read_next() {
        m_current = m_next;
        m_cursor  = MSB;

        read_next_from_stream();
    }

    struct BitSink {
        BitIStream *m_ptr;

        inline auto read_bit() -> uint8_t { return m_ptr->read_bit(); }

        template<class T>
        inline auto read_int(size_t amount = sizeof(T) * CHAR_BIT) -> T {
            return m_ptr->template read_int<T>(amount);
        }
    };

    inline auto bit_sink() -> BitSink { return BitSink{this}; }

  public:
    /// \brief Constructs a bitwise input stream.
    ///
    /// \param input The underlying input stream.
    inline BitIStream(std::istream &&input) : m_stream(input) {
        char c;
        if (m_stream.get(c)) {
            // prepare the state by reading the first byte into to the `m_next`
            // member. `read_next()` will then shift it to the `m_current`
            // member, from which the `read_XXX()` methods take away bits.

            m_is_final = false;
            m_next     = c;

            read_next();
        } else {
            // special case: if the stream is empty to begin with, we
            // never read the last 3 bits and just treat it as completely empty

            m_is_final   = true;
            m_final_bits = 0;
        }
    }

    BitIStream(BitIStream &&other) = default;

    /// TODO document
    inline auto eof() const -> bool {
        // If there are no more bytes, and all bits from the current buffer are read,
        // we are done
        return m_is_final && (m_cursor <= (MSB - m_final_bits));
    }

    /// \brief Reads the next single bit from the input.
    /// \return 1 if the next bit is set, 0 otherwise.
    inline auto read_bit() -> uint8_t {
        if (!eof()) {
            uint8_t bit = (m_current >> m_cursor) & 1;
            if (m_cursor) {
                --m_cursor;
            } else {
                read_next();
            }

            ++m_bits_read;
            return bit;
        } else {
            return 0; // EOF
        }
    }

    /// \brief Reads the integer value of the next \c amount bits in MSB first
    ///        order.
    /// \tparam The integer type to read.
    /// \param bits The bit width of the integer to read. By default, this
    ///             equals the bit width of type \c T.
    /// \return The integer value of the next \c amount bits in MSB first
    ///         order.
    template<class T>
    inline auto read_int(size_t bits = sizeof(T) * CHAR_BIT) -> T {

        const size_t bits_left_in_current = m_cursor + 1ULL;
        if (bits < bits_left_in_current) {
            auto sink = bit_sink();

            // we are reading only few bits
            // simply use the bit-by-bit method
            T value = 0;
            for (size_t i = 0; i < bits; i++) {
                value <<= 1;
                value |= sink.read_bit();
            }
            return value;
        } else {
            // we are at least consuming the current byte
            const size_t in_bits = bits;

            bits -= bits_left_in_current;
            size_t v = m_current & ((1ULL << bits_left_in_current) - 1ULL);
            v <<= bits;

            // read as many full bytes as possible
            if (bits >= 8ULL) {
                if (bits >= 16ULL) {
                    // use m_next and then read remaining bytes
                    const size_t n = (bits / 8ULL) - 1;
                    bits %= 8ULL;

                    const size_t off = sizeof(size_t) - n;

                    size_t v_bytes = 0;
                    m_stream.read(((char *) &v_bytes) + off, n);

// convert full bytes into LITTLE ENDIAN (!) representation
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    v_bytes = __builtin_bswap64(v_bytes);
#endif

                    v_bytes |= size_t(m_next) << (n * 8ULL);
                    v |= (v_bytes) << bits;

                    // keep data consistency
                    read_next_from_stream();
                } else {
                    // use m_next
                    bits -= 8ULL;
                    read_next();
                    v |= size_t(m_current) << bits;
                }
            }

            // get next byte
            read_next();

            // read remaining bits
            // they (must) be available in the next byte, so just mask them out
            if (bits) {
                v |= (m_current >> (8ULL - bits));
                m_cursor = MSB - bits;
            }

            m_bits_read += in_bits;
            return v;
        }
    }

    inline auto bits_read() const -> size_t { return m_bits_read; }
};

} // namespace gracli
