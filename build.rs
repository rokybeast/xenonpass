use std::env;

fn main() {
    pkg_config::Config::new().probe("libsodium").unwrap();
    cc::Build::new()
        .file("vault/kdf.c")
        .file("vault/crypto.c")
        .file("vault/io.c")
        .file("vault/mem.c")
        .include("vault/include")
        .compile("xpass");
}
