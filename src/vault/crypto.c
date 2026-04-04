#include "xpass.h"

int xpass_encrypt(uint8_t *ciphertext, unsigned long long *ciphertext_len,
                  const uint8_t *plaintext, size_t plaintext_len,
                  uint8_t nonce[XPASS_NONCE_LEN],
                  const uint8_t key[XPASS_KEY_LEN]) {
    randombytes_buf(nonce, XPASS_NONCE_LEN);
    
    if (crypto_aead_aes256gcm_is_available() == 1) {
        ciphertext[0] = XPASS_CIPHER_AESGCM;
        unsigned long long aes_ct_len = 0;
        int ret = crypto_aead_aes256gcm_encrypt(
            ciphertext + 1, &aes_ct_len,
            plaintext, plaintext_len,
            NULL, 0,
            NULL, nonce, key);
        *ciphertext_len = aes_ct_len + 1;
        return ret;
    } else {
        ciphertext[0] = XPASS_CIPHER_XCHACHA;
        unsigned long long xchacha_ct_len = 0;
        int ret = crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext + 1, &xchacha_ct_len,
            plaintext, plaintext_len,
            NULL, 0,
            NULL, nonce, key);
        *ciphertext_len = xchacha_ct_len + 1;
        return ret;
    }
}

int xpass_decrypt(uint8_t *plaintext, unsigned long long *plaintext_len,
                  const uint8_t *ciphertext, size_t ciphertext_len,
                  const uint8_t nonce[XPASS_NONCE_LEN],
                  const uint8_t key[XPASS_KEY_LEN]) {
    if (ciphertext_len == 0) return -1;
    
    uint8_t cipher_id = ciphertext[0];
    
    if (cipher_id == XPASS_CIPHER_AESGCM) {
        if (crypto_aead_aes256gcm_is_available() == 0) {
            return -1;
        }
        return crypto_aead_aes256gcm_decrypt(
            plaintext, plaintext_len,
            NULL,
            ciphertext + 1, ciphertext_len - 1,
            NULL, 0,
            nonce, key);
    } else if (cipher_id == XPASS_CIPHER_XCHACHA) {
        return crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext, plaintext_len,
            NULL,
            ciphertext + 1, ciphertext_len - 1,
            NULL, 0,
            nonce, key);
    }
    
    return -1;
}
