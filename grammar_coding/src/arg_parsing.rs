use std::ffi::CString;

use clap::Parser;
use libc::c_char;

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct ArgsInternal {
    #[clap(short, long, help = "The input file", required = true)]
    file: String,
    #[clap(
        short,
        long,
        parse(from_flag),
        help = "Starts gracli in interactive mode which allows multiple accesses to a grammar compressed file (unimplemented)"
    )]
    interactive: bool,
    #[clap(
        short,
        long,
        parse(from_flag),
        help = "Decodes an encoded grammar file and outputs the resulting string to stdout (default behavior)"
    )]
    decode: bool,
}

#[repr(C)]
pub struct RawArgs {
    file: *mut c_char,
    interactive: bool,
    decode: bool,
}

impl From<ArgsInternal> for RawArgs {
    fn from(int: ArgsInternal) -> Self {
        println!("Parsed string: {}", &int.file);
        let cstr: CString =
            CString::new(int.file).expect("Error converting file name to C compatible string");
        let raw_file = cstr.as_ptr() as *mut c_char;
        // We don't want the string to be deallocated when passing it to C++
        std::mem::forget(cstr);

        println!("Is null? {}", raw_file.is_null());

        Self {
            file: raw_file,
            interactive: int.interactive,
            decode: int.decode,
        }
    }
}

#[no_mangle]
pub extern "C" fn parse_args() -> *const RawArgs {
    let args = ArgsInternal::parse().into();
    let arg_ptr = &args as *const RawArgs;
    std::mem::forget(args);
    arg_ptr
}
