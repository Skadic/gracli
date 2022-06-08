use crate::{util::RawVec, consts::RULE_OFFSET};


pub unsafe fn encode<Out: std::io::Write>(
    grammar: &RawVec<RawVec<u32>>,
    mut out: Out,
) -> Result<(), std::io::Error> {
    let rule_count = grammar.len;
    for (i, rule) in (0..rule_count).map(|i| &*grammar.ptr.add(i)).enumerate() {
        write!(out, "R{i} -> ")?;
        for symbol in (0..rule.len).map(|i| *rule.ptr.add(i)) {
            if symbol < RULE_OFFSET as u32 {
                let symbol = symbol as u8 as char;
                write!(out, "{symbol} ")?;
            } else {
                let rule_id = symbol as u32 - RULE_OFFSET;
                write!(out, "R{rule_id} ")?;
            }
        }
        writeln!(out)?;
    }

    Ok(())
}
