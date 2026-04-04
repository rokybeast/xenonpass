#![allow(dead_code)]
#![allow(non_camel_case_types)]

use core::ffi::{c_char, c_int, c_ulonglong, c_void};

pub const XPASS_KEY_LEN: usize = 32;
pub const XPASS_SALT_LEN: usize = 16;
pub const XPASS_NONCE_LEN: usize = 24;
pub const XPASS_TAG_LEN: usize = 16;
pub const XPASS_MAGIC: &[u8; 4] = b"XPAS";
pub const XPASS_MAGIC_LEN: usize = 4;
pub const XPASS_VERSION: u16 = 1;

#[repr(C, packed)]
pub struct xpass_header_t {
    pub magic: [u8; XPASS_MAGIC_LEN],
    pub version: u16,
    pub salt: [u8; XPASS_SALT_LEN],
    pub nonce: [u8; XPASS_NONCE_LEN],
    pub ciphertext_len: u32,
}

unsafe extern "C" {
    pub fn xpass_init() -> c_int;

    pub fn xpass_derive_key(
        key: *mut u8,
        password: *const c_char,
        password_len: usize,
        salt: *const u8,
    ) -> c_int;

    pub fn xpass_generate_salt(salt: *mut u8);

    pub fn xpass_encrypt(
        ciphertext: *mut u8,
        ciphertext_len: *mut c_ulonglong,
        plaintext: *const u8,
        plaintext_len: usize,
        nonce: *mut u8,
        key: *const u8,
    ) -> c_int;

    pub fn xpass_decrypt(
        plaintext: *mut u8,
        plaintext_len: *mut c_ulonglong,
        ciphertext: *const u8,
        ciphertext_len: usize,
        nonce: *const u8,
        key: *const u8,
    ) -> c_int;

    pub fn xpass_vault_save(
        path: *const c_char,
        ciphertext: *const u8,
        ciphertext_len: u32,
        salt: *const u8,
        nonce: *const u8,
    ) -> c_int;

    pub fn xpass_vault_load(
        path: *const c_char,
        ciphertext: *mut *mut u8,
        ciphertext_len: *mut u32,
        salt: *mut u8,
        nonce: *mut u8,
    ) -> c_int;

    pub fn xpass_secure_zero(buf: *mut c_void, len: usize);
}
