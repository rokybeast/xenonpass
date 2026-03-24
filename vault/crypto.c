#include "xpass.h"

int xpass_encrypt(uint8_t *ciphertext, unsigned long long *ciphertext_len,
                  const uint8_t *plaintext, size_t plaintext_len,
                  uint8_t nonce[XPASS_NONCE_LEN],
                  const uint8_t key[XPASS_KEY_LEN]) {
    randombytes_buf(nonce, XPASS_NONCE_LEN);
    return crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext, ciphertext_len,
        plaintext, plaintext_len,
        NULL, 0,
        NULL, nonce, key);
}

int xpass_decrypt(uint8_t *plaintext, unsigned long long *plaintext_len,
                  const uint8_t *ciphertext, size_t ciphertext_len,
                  const uint8_t nonce[XPASS_NONCE_LEN],
                  const uint8_t key[XPASS_KEY_LEN]) {
    return crypto_aead_xchacha20poly1305_ietf_decrypt(
        plaintext, plaintext_len,
        NULL,
        ciphertext, ciphertext_len,
        NULL, 0,
        nonce, key);
}
