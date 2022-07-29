use std::ffi::CString;

use clap::Parser;
use libc::c_char;



// TODO Add ability to read the source string from a file for validation
#[derive(Parser, Debug)]
#[clap(name = "gracli", author, version, about, long_about = None)]
struct ArgsInternal {
    #[clap(short, long, help = "The input file", value_name("PATH"), required = true)]

    file: String,
    #[clap(
        short,
        long,
        parse(from_flag),
        group("action"),
        help = "Decodes an encoded grammar file and outputs the resulting string to stdout (default behavior)"
    )]
    decode: bool,
    #[clap(
        short('r'),
        long,
        group("action"),
        value_name("NUM_QUERIES"),
        help = "Benchmarks runtime of a Grammar's random access queries. Value is the number of queries."
    )]
    benchmark_random_access: Option<usize>,
    #[clap(
        short('s'),
        long,
        group("action"),
        value_name("NUM_QUERIES"),
        help = "Benchmarks runtime of a Grammar's substring queries. Value is the number of queries."
    )]
    benchmark_substring: Option<usize>,
    #[clap(
        short('l'),
        long,
        value_name("LENGTH"),
        help = "Length of the substrings while benchmarking substring queries."
    )]
    substring_length: Option<usize>,
    #[clap(
        short('g'),
        long,
        arg_enum,
        default_value("sampled-scan6400"),
        value_name("GRAMMAR_TYPE"),
        help = "The Grammar Access Data Structure to use"
    )]
    grammar_type: GrammarType,
}

#[repr(C)]
#[derive(clap::ArgEnum, Debug, Clone, Copy, PartialEq, Eq)]
pub enum GrammarType {
    Naive,
    SampledScan512,
    SampledScan6400,
    SampledScan12800,
}

#[repr(C)]
pub struct RawArgs {
    file: *mut c_char,
    //interactive: bool,
    decode: bool,
    benchmark_random_access: usize,
    benchmark_substring: usize,
    substring_length: usize,
    grammar_type: GrammarType
}

impl From<ArgsInternal> for RawArgs {
    fn from(int: ArgsInternal) -> Self {
        let cstr: CString =
            CString::new(int.file).expect("Error converting file name to C compatible string");
        let raw_file = cstr.into_raw() as *mut c_char;

        Self {
            file: raw_file,
            //interactive: int.interactive,
            decode: int.decode,
            benchmark_random_access: int.benchmark_random_access.unwrap_or(0),
            benchmark_substring: int.benchmark_substring.unwrap_or(0),
            substring_length: int.substring_length.unwrap_or(0),
            grammar_type: int.grammar_type,
        }
    }
}

#[no_mangle]
pub extern "C" fn parse_args() -> *const RawArgs {
    let args = Box::<RawArgs>::new(ArgsInternal::parse().into());
    Box::into_raw(args)
}

#[no_mangle]
pub unsafe extern "C" fn dealloc_raw_args(raw_args: *mut RawArgs) {
    if raw_args.is_null() {
        return;
    }

    // When this goes out of scope, the heap memory is deallocated
    let raw_args = Box::from_raw(raw_args);
    // When this goes out of scope, the file string is deallocated
    let _ = CString::from_raw(raw_args.file);
}
