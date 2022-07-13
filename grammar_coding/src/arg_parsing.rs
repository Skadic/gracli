use std::ffi::CString;

use clap::Parser;
use libc::c_char;

#[derive(Parser, Debug)]
#[clap(name = "gracli", author, version, about, long_about = None)]
struct ArgsInternal {
    #[clap(short, long, help = "The input file", required = true)]
    file: String,
    /*#[clap(
        short,
        long,
        parse(from_flag),
        help = "Starts gracli in interactive mode which allows multiple accesses to a grammar compressed file (unimplemented)"
    )]*/
    interactive: bool,
    #[clap(
        short,
        long,
        parse(from_flag),
        help = "Decodes an encoded grammar file and outputs the resulting string to stdout (default behavior)"
    )]
    decode: bool,
    #[clap(
        short,
        long,
        help = "Runs a given number of random accesses on the input grammar to test and compares it against substring queries on the source string."
    )]
    validate: Option<usize>,
}

#[repr(C)]
pub struct RawArgs {
    file: *mut c_char,
    //interactive: bool,
    decode: bool,
    validate: isize,
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
            validate: int.validate.map(|v| v as isize).unwrap_or(-1),
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
