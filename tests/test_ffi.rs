#[path = "../src/ffi/mod.rs"]
mod ffi;

use ffi::*;
use std::ffi::CString;


#[test]
fn test_encrypt_decrypt_roundtrip() {
    unsafe {
        assert!(xpass_init() >= 0);

        let mut key = [0u8; XPASS_KEY_LEN];
        let mut salt = [0u8; XPASS_SALT_LEN];
        xpass_generate_salt(salt.as_mut_ptr());

        let password = b"xenon-master-key-2024\0";
        xpass_derive_key(key.as_mut_ptr(), password.as_ptr() as *const i8, password.len() - 1, salt.as_ptr());

        let plaintext = b"user:admin\npass:s3cret\nnotes:vault entry\0";
        let pt_len = plaintext.len() - 1;

        let mut ciphertext = vec![0u8; pt_len + XPASS_TAG_LEN];
        let mut ct_len: std::ffi::c_ulonglong = 0;
        let mut nonce = [0u8; XPASS_NONCE_LEN];

        let enc_ret = xpass_encrypt(
            ciphertext.as_mut_ptr(),
            &mut ct_len,
            plaintext.as_ptr(),
            pt_len,
            nonce.as_mut_ptr(),
            key.as_ptr(),
        );
        assert_eq!(enc_ret, 0);
        assert_eq!(ct_len as usize, pt_len + XPASS_TAG_LEN);

        let mut decrypted = vec![0u8; pt_len];
        let mut dec_len: std::ffi::c_ulonglong = 0;
        let dec_ret = xpass_decrypt(
            decrypted.as_mut_ptr(),
            &mut dec_len,
            ciphertext.as_ptr(),
            ct_len as usize,
            nonce.as_ptr(),
            key.as_ptr(),
        );
        assert_eq!(dec_ret, 0);
        assert_eq!(dec_len as usize, pt_len);
        assert_eq!(&decrypted[..], &plaintext[..pt_len]);

        xpass_secure_zero(key.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
    }
}

#[test]
fn test_kdf_consistency() {
    unsafe {
        assert!(xpass_init() >= 0);

        let mut salt = [0u8; XPASS_SALT_LEN];
        xpass_generate_salt(salt.as_mut_ptr());

        let password = b"consistent-derivation-test\0";
        let mut key1 = [0u8; XPASS_KEY_LEN];
        let mut key2 = [0u8; XPASS_KEY_LEN];

        xpass_derive_key(key1.as_mut_ptr(), password.as_ptr() as *const i8, password.len() - 1, salt.as_ptr());
        xpass_derive_key(key2.as_mut_ptr(), password.as_ptr() as *const i8, password.len() - 1, salt.as_ptr());

        assert_eq!(key1, key2);

        let mut salt2 = [0u8; XPASS_SALT_LEN];
        xpass_generate_salt(salt2.as_mut_ptr());
        let mut key3 = [0u8; XPASS_KEY_LEN];
        xpass_derive_key(key3.as_mut_ptr(), password.as_ptr() as *const i8, password.len() - 1, salt2.as_ptr());
        assert_ne!(key1, key3);

        xpass_secure_zero(key1.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
        xpass_secure_zero(key2.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
        xpass_secure_zero(key3.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
    }
}

#[test]
fn test_file_io_integrity() {
    unsafe {
        assert!(xpass_init() >= 0);

        let mut key = [0u8; XPASS_KEY_LEN];
        let mut salt = [0u8; XPASS_SALT_LEN];
        xpass_generate_salt(salt.as_mut_ptr());

        let password = b"file-io-test-password\0";
        xpass_derive_key(key.as_mut_ptr(), password.as_ptr() as *const i8, password.len() - 1, salt.as_ptr());

        let plaintext = b"site:github.com\nuser:xenon\npass:ultraS3cure!\0";
        let pt_len = plaintext.len() - 1;

        let mut ciphertext = vec![0u8; pt_len + XPASS_TAG_LEN];
        let mut ct_len: std::ffi::c_ulonglong = 0;
        let mut nonce = [0u8; XPASS_NONCE_LEN];

        xpass_encrypt(
            ciphertext.as_mut_ptr(),
            &mut ct_len,
            plaintext.as_ptr(),
            pt_len,
            nonce.as_mut_ptr(),
            key.as_ptr(),
        );

        let vault_path = CString::new("/tmp/test_vault_ffi.xpass").unwrap();
        let save_ret = xpass_vault_save(
            vault_path.as_ptr(),
            ciphertext.as_ptr(),
            ct_len as u32,
            salt.as_ptr(),
            nonce.as_ptr(),
        );
        assert_eq!(save_ret, 0);

        let mut loaded_ct: *mut u8 = std::ptr::null_mut();
        let mut loaded_ct_len: u32 = 0;
        let mut loaded_salt = [0u8; XPASS_SALT_LEN];
        let mut loaded_nonce = [0u8; XPASS_NONCE_LEN];

        let load_ret = xpass_vault_load(
            vault_path.as_ptr(),
            &mut loaded_ct,
            &mut loaded_ct_len,
            loaded_salt.as_mut_ptr(),
            loaded_nonce.as_mut_ptr(),
        );
        assert_eq!(load_ret, 0);
        assert_eq!(loaded_ct_len, ct_len as u32);
        
        let loaded_ct_slice = std::slice::from_raw_parts(loaded_ct, loaded_ct_len as usize);
        assert_eq!(loaded_ct_slice, &ciphertext[..ct_len as usize]);
        assert_eq!(loaded_salt, salt);
        assert_eq!(loaded_nonce, nonce);

        let mut decrypted = vec![0u8; pt_len];
        let mut dec_len: std::ffi::c_ulonglong = 0;
        let dec_ret = xpass_decrypt(
            decrypted.as_mut_ptr(),
            &mut dec_len,
            loaded_ct,
            loaded_ct_len as usize,
            loaded_nonce.as_ptr(),
            key.as_ptr(),
        );
        assert_eq!(dec_ret, 0);
        assert_eq!(&decrypted[..], &plaintext[..pt_len]);

        libc::free(loaded_ct as *mut _);
        libc::remove(vault_path.as_ptr());
        xpass_secure_zero(key.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
    }
}

#[test]
fn test_wrong_password() {
    unsafe {
        assert!(xpass_init() >= 0);

        let mut salt = [0u8; XPASS_SALT_LEN];
        xpass_generate_salt(salt.as_mut_ptr());

        let correct_password = b"correct-horse-battery\0";
        let wrong_password = b"wrong-password-attempt\0";

        let mut correct_key = [0u8; XPASS_KEY_LEN];
        let mut wrong_key = [0u8; XPASS_KEY_LEN];

        xpass_derive_key(correct_key.as_mut_ptr(), correct_password.as_ptr() as *const i8, correct_password.len() - 1, salt.as_ptr());
        xpass_derive_key(wrong_key.as_mut_ptr(), wrong_password.as_ptr() as *const i8, wrong_password.len() - 1, salt.as_ptr());

        let plaintext = b"secret-data-that-must-stay-safe\0";
        let pt_len = plaintext.len() - 1;

        let mut ciphertext = vec![0u8; pt_len + XPASS_TAG_LEN];
        let mut ct_len: std::ffi::c_ulonglong = 0;
        let mut nonce = [0u8; XPASS_NONCE_LEN];

        xpass_encrypt(
            ciphertext.as_mut_ptr(),
            &mut ct_len,
            plaintext.as_ptr(),
            pt_len,
            nonce.as_mut_ptr(),
            correct_key.as_ptr(),
        );

        let mut decrypted = vec![0u8; pt_len];
        let mut dec_len: std::ffi::c_ulonglong = 0;
        let dec_ret = xpass_decrypt(
            decrypted.as_mut_ptr(),
            &mut dec_len,
            ciphertext.as_ptr(),
            ct_len as usize,
            nonce.as_ptr(),
            wrong_key.as_ptr(),
        );
        assert_ne!(dec_ret, 0);

        xpass_secure_zero(correct_key.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
        xpass_secure_zero(wrong_key.as_mut_ptr() as *mut _, XPASS_KEY_LEN);
    }
}

#[test]
fn test_secure_zero() {
    unsafe {
        let mut buf = [0xAAu8; 64];
        xpass_secure_zero(buf.as_mut_ptr() as *mut _, buf.len());

        let mut all_zero = true;
        for i in 0..buf.len() {
            if buf[i] != 0 {
                all_zero = false;
                break;
            }
        }
        assert!(all_zero);
    }
}
