use core::slice;
use std::io::Read;

use bitstream_io::{LittleEndian, BitReader, BitRead};

const RULE_OFFSET: u32 = 256;

#[repr(C)]
pub struct RawVec<T> {
    ptr: *mut T,
    len: usize,
}

#[no_mangle]
pub unsafe extern fn decode_bytes(ptr: *const u8, len: usize) -> RawVec<RawVec<u32>> {
    let slice = slice::from_raw_parts(ptr, len);
    let rules: Vec<Vec<u32>> = decode(slice).unwrap();
    let mut rules = rules.into_iter().map(|mut rule| {
        let ptr = rule.as_mut_ptr();
        let len = rule.len();
        std::mem::forget(rule);

        RawVec { ptr, len }
    }).collect::<Vec<_>>();

    let ptr = rules.as_mut_ptr();
    let len = rules.len();
    std::mem::forget(rules);

    RawVec { ptr, len }
}

fn decode(input: impl Read) -> Result<Vec<Vec<u32>>, std::io::Error> {
    let mut bit_reader = BitReader::endian(input, LittleEndian);

    let mut buf32 = [0u8; 4];
    let mut buf8 = [0u8];

    macro_rules! rd {
        (u32) => {{
            bit_reader.read_bytes(&mut buf32)?;
            u32::from_le_bytes(buf32) as u32 
        }};
        (u8) => {{
            bit_reader.read_bytes(&mut buf8)?;
            buf8[0] as u32
        }};
    }

    let rule_count = rd!(u32) as usize;

    let mut rules = Vec::with_capacity(rule_count);

    for _ in 0..rule_count {
        let rule_size = rd!(u32) as usize;
        let mut rule = Vec::<u32>::with_capacity(rule_size);
        for _ in 0..rule_size {
            let is_nonterminal = bit_reader.read_bit()?;
            let symbol = if is_nonterminal {
                rd!(u32) + RULE_OFFSET
            } else {
                rd!(u8)
            };
            rule.push(symbol);
        }
        rule.shrink_to_fit();
        rules.push(rule);
    }
    rules.shrink_to_fit();
    Ok(rules)
}
