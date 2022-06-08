use crate::{consts::RULE_OFFSET, util::RawVec};
use bitstream_io::{BigEndian, BitRead, BitReader, BitWrite, BitWriter};
use std::io::Read;


pub fn decode(input: impl Read) -> Result<Vec<Vec<u32>>, std::io::Error> {
    let mut bit_reader = BitReader::endian(input, BigEndian);

    let mut buf32 = [0u8; 4];
    let mut buf8 = [0u8];

    macro_rules! rd {
        (u32) => {{
            bit_reader.read_bytes(&mut buf32)?;
            u32::from_be_bytes(buf32) as u32
        }};
        (u8) => {{
            bit_reader.read_bytes(&mut buf8)?;
            buf8[0] as u32
        }};
    }

    let rule_count = rd!(u32);
    let min_rule_len = rd!(u32);
    let _max_rule_len = rd!(u32);

    let mut rules = Vec::with_capacity(rule_count as usize);

    for _ in 0..rule_count {
        let rule_len = rd!(u32) + min_rule_len;
        let mut rule = Vec::<u32>::with_capacity(rule_len as usize);
        for _ in 0..rule_len {
            let is_nonterminal = bit_reader.read_bit()?;
            let symbol = if is_nonterminal {
                rd!(u32) + RULE_OFFSET
            } else {
                rd!(u8)
            };
            rule.push(symbol as u32);
        }
        rule.shrink_to_fit();
        rules.push(rule);
    }
    rules.shrink_to_fit();
    Ok(rules)
}

pub unsafe fn encode<Out: std::io::Write>(
    grammar: &RawVec<RawVec<u32>>,
    out: Out,
) -> Result<(), std::io::Error> {
    let mut bit_writer = BitWriter::endian(out, BigEndian);

    // We write this to the output so we know when to stop reading, in case there are
    // additional padding bits
    let rule_count = grammar.len;
    bit_writer.write_bytes(&u32::to_be_bytes(rule_count as u32))?;

    let (min_len, max_len) = (0..rule_count)
        .map(|i| &*grammar.ptr.add(i))
        .map(|raw_rule| raw_rule.len as u32)
        .fold((u32::MAX, 0), |(old_min, old_max), len| {
            (u32::min(old_min, len), u32::max(old_max, len))
        });

    bit_writer.write_bytes(&u32::to_be_bytes(min_len))?;
    bit_writer.write_bytes(&u32::to_be_bytes(max_len))?;

    for rule in (0..rule_count).map(|i| &*grammar.ptr.add(i)) {
        // Write the rule length to the output
        let encoded_rule_size = rule.len as u32 - min_len;
        bit_writer.write_bytes(&u32::to_be_bytes(encoded_rule_size))?;

        for symbol in (0..rule.len).map(|i| *rule.ptr.add(i)) {
            if symbol < RULE_OFFSET as u32 {
                // If the symbol is a terminal we write a 0 bit and then the terminal itself
                let symbol = symbol as u8;
                bit_writer.write_bit(false)?;
                bit_writer.write_bytes(&[symbol])?;
            } else {
                // If the symbol is a non-terminal we write a 1 bit and then the symbol
                let symbol_bytes = u32::to_be_bytes(symbol as u32 - RULE_OFFSET);
                bit_writer.write_bit(true)?;
                bit_writer.write_bytes(&symbol_bytes)?;
            }
        }
    }

    // Make sure any remaining bits are also written out
    bit_writer.byte_align()?;
    bit_writer.flush()?;

    Ok(())
}
