use std::env;

fn main() {
    pkg_config::Config::new().probe("libsodium").unwrap();
    cc::Build::new()
        .file("src/vault/kdf.c")
        .file("src/vault/crypto.c")
        .file("src/vault/io.c")
        .file("src/vault/mem.c")
        .include("src/vault/include")
        .compile("xpass");
}
