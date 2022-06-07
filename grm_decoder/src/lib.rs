use std::{io::Read, slice};

use bitstream_io::{BigEndian, BitRead, BitReader, BitWrite, BitWriter};

const RULE_OFFSET: u32 = 256;

#[repr(C)]
/// Represents a vector with a pointer and the number of elements.
/// Note that the allocated memory is not deleted automatically!
pub struct RawVec<T> {
    pub ptr: *mut T,
    pub len: usize,
}

/// Decodes an encoded grammar from a vector of bytes.
///
/// # Arguments
///
/// * `ptr` - The pointer to the start of the bytes from which to decode the grammar
/// * `len` - The length of the input string
///
/// # Safety
///
/// This function is safe if the pointer and length making up the input are valid memory and the
/// bytes depict a valid encoded grammar.
///
#[no_mangle]
pub unsafe extern "C" fn decode_bytes(ptr: *const u8, len: usize) -> RawVec<RawVec<u32>> {
    let slice = slice::from_raw_parts(ptr, len);
    let rules: Vec<Vec<u32>> = decode(slice).expect("Error decoding grammar");
    let mut rules = rules
        .into_iter()
        .map(|mut rule| {
            let ptr = rule.as_mut_ptr();
            let len = rule.len();
            std::mem::forget(rule);

            RawVec { ptr, len }
        })
        .collect::<Vec<_>>();

    let ptr = rules.as_mut_ptr();
    let len = rules.len();
    std::mem::forget(rules);

    RawVec { ptr, len }
}

/// This function takes a grammar in raw vec form and encodes it to a vector of bytes.
///
/// # Arguments
///
/// * `grammar` - The grammar in raw form that is to be encoded. Note that this grammar is expected
/// to be renumbered! In addition, the original string may not contain any null bytes.
///
/// # Safety
///
/// This function is safe if the pointers and the specified lengths in the raw vecs are valid and
/// the vecs describe a valid grammar.
///
#[no_mangle]
pub unsafe extern "C" fn encode_grammar_to_byte_vec(grammar: RawVec<RawVec<u32>>) -> RawVec<u8> {
    let mut s = Vec::<u8>::new();
    encode(&grammar, &mut s).expect("Error encoding grammar to string");
    s.shrink_to_fit();
    let ptr = s.as_mut_ptr();
    let len = s.len();
    std::mem::forget(s);

    RawVec { ptr, len }
}

/// Encodes a grammar and writes the contents to a file.
///
/// # Arguments
///
/// * `grammar` - The grammar in raw form that is to be encoded. Note that this grammar is expected
/// to be renumbered! In addition, the original string may not contain any null bytes.
/// * `ptr` - The pointer to the string that contains the file name. This must be valid utf8.
/// * `len` - The length of the file name string in bytes.
///
/// # Safety
///
/// For this function to be safe, the raw vecs must point to valid memory and must represent a
/// valid grammar. Besides that the pointer and length of the file name must constitute a valid
/// utf8 string.
///
#[no_mangle]
pub unsafe extern "C" fn encode_grammar_to_file(
    grammar: RawVec<RawVec<u32>>,
    ptr: *const u8,
    len: usize,
) {
    let file_name_bytes = slice::from_raw_parts(ptr as *mut u8, len);
    let file_name = String::from_utf8_lossy(file_name_bytes);

    let file = std::fs::File::create(&file_name[..]).expect("Could not create file");

    encode(&grammar, file).expect("Error decoding Grammar")
}

fn decode(input: impl Read) -> Result<Vec<Vec<u32>>, std::io::Error> {
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

unsafe fn encode<Out: std::io::Write>(
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
