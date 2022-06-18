#[allow(unreachable_patterns)]
mod coding;
mod consts;
mod grammar_tuple;
mod didactic;
mod util;
mod arg_parsing;

pub use coding::{to_file, to_bytes, from_bytes};
pub use arg_parsing::parse_args;
