use std::path::PathBuf;

use cbindgen::Config;

fn main() {
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR")
        .map(PathBuf::from)
        .unwrap();
    let package_name = std::env::var("CARGO_PKG_NAME").unwrap();
    let include_dir = crate_dir.join("include");
    let include_file = include_dir.join(format!("{package_name}.hpp"));

    if !include_dir.exists() {
        std::fs::create_dir(include_dir).unwrap();
    }

    let config = Config::from_file(crate_dir.join("cbindgen.toml")).unwrap();

    cbindgen::generate_with_config(&crate_dir, config)
        .unwrap()
        .write_to_file(include_file);
}
