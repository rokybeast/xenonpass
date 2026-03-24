#include "xpass.h"

int xpass_derive_key(uint8_t key[XPASS_KEY_LEN],
                     const char *password, size_t password_len,
                     const uint8_t salt[XPASS_SALT_LEN]) {
    return crypto_pwhash(key, XPASS_KEY_LEN,
                         password, password_len,
                         salt,
                         crypto_pwhash_OPSLIMIT_MODERATE,
                         crypto_pwhash_MEMLIMIT_MODERATE,
                         crypto_pwhash_ALG_ARGON2ID13);
}

void xpass_generate_salt(uint8_t salt[XPASS_SALT_LEN]) {
    randombytes_buf(salt, XPASS_SALT_LEN);
}
