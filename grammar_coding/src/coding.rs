use std::slice;

use crate::{util::RawVec, grammar_tuple, didactic};


#[repr(u32)]
#[derive(Clone, Copy)]
#[non_exhaustive]
pub enum Coder {
    Didactic,
    GrammarTuple
}

type RawGrammar = RawVec<RawVec<u32>>;

/// Decodes an encoded grammar from a vector of bytes.
///
/// # Arguments
///
/// * `ptr` - The pointer to the start of the bytes from which to decode the grammar
/// * `len` - The length of the input string
/// * `coder` - The coder used to decode the grammar.
/// # Safety
///
/// This function is safe if the pointer and length making up the input are valid memory and the
/// bytes depict a valid encoded grammar.
///
#[no_mangle]
pub unsafe extern fn from_bytes(ptr: *const u8, len: usize, coder: Coder) -> *mut RawGrammar
{
    let slice = slice::from_raw_parts(ptr, len);
    let rules: Vec<Vec<u32>> = match coder {
        Coder::Didactic => unimplemented!("Didactic coder does not decode"),
        Coder::GrammarTuple => grammar_tuple::decode(slice),
        _ => unimplemented!("Coder unimplemented")
    }.expect("Error decoding Grammar");

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

    let raw_v = Box::new(RawVec { ptr, len });
    let raw_v_ptr = Box::into_raw(raw_v);
    raw_v_ptr
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
pub unsafe extern fn to_bytes(grammar: RawGrammar, coder: Coder) -> *mut RawVec<u8> {
    let mut s = Vec::<u8>::new();
    match coder {
        Coder::Didactic => didactic::encode(&grammar, &mut s),
        Coder::GrammarTuple => grammar_tuple::encode(&grammar, &mut s),
        _ => unimplemented!("Coder unimplemented")
    }.expect("Error encoding grammar to string");
    s.shrink_to_fit();
    let ptr = s.as_mut_ptr();
    let len = s.len();
    std::mem::forget(s);

    let raw_v  = Box::new(RawVec { ptr, len });
    let raw_v_ptr = Box::into_raw(raw_v);
    raw_v_ptr
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
pub unsafe extern fn to_file(grammar: RawGrammar, ptr: *const u8, len: usize, coder: Coder) {
    let file_name_bytes = slice::from_raw_parts(ptr as *mut u8, len);
    let file_name = String::from_utf8_lossy(file_name_bytes);

    let file = std::fs::File::create(&file_name[..]).expect("Could not create file");

    match coder {
        Coder::Didactic => didactic::encode(&grammar, file),
        Coder::GrammarTuple => grammar_tuple::encode(&grammar, file),
        _ => unimplemented!("Coder unimplemented")
    }.expect("Error decoding Grammar")
}
