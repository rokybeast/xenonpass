#ifndef XPASS_H
#define XPASS_H

#include <stddef.h>
#include <stdint.h>
#include <sodium.h>

#define XPASS_KEY_LEN crypto_aead_xchacha20poly1305_ietf_KEYBYTES
#define XPASS_SALT_LEN crypto_pwhash_SALTBYTES
#define XPASS_NONCE_LEN crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
#define XPASS_TAG_LEN crypto_aead_xchacha20poly1305_ietf_ABYTES
#define XPASS_MAGIC "XPAS"
#define XPASS_MAGIC_LEN 4
#define XPASS_VERSION 1

typedef struct {
    uint8_t magic[XPASS_MAGIC_LEN];
    uint16_t version;
    uint8_t salt[XPASS_SALT_LEN];
    uint8_t nonce[XPASS_NONCE_LEN];
    uint32_t ciphertext_len;
} __attribute__((packed)) xpass_header_t;

int xpass_init(void);

int xpass_derive_key(uint8_t key[XPASS_KEY_LEN],
                     const char *password, size_t password_len,
                     const uint8_t salt[XPASS_SALT_LEN]);

void xpass_generate_salt(uint8_t salt[XPASS_SALT_LEN]);

int xpass_encrypt(uint8_t *ciphertext, unsigned long long *ciphertext_len,
                  const uint8_t *plaintext, size_t plaintext_len,
                  uint8_t nonce[XPASS_NONCE_LEN],
                  const uint8_t key[XPASS_KEY_LEN]);

int xpass_decrypt(uint8_t *plaintext, unsigned long long *plaintext_len,
                  const uint8_t *ciphertext, size_t ciphertext_len,
                  const uint8_t nonce[XPASS_NONCE_LEN],
                  const uint8_t key[XPASS_KEY_LEN]);

int xpass_vault_save(const char *path,
                     const uint8_t *ciphertext, uint32_t ciphertext_len,
                     const uint8_t salt[XPASS_SALT_LEN],
                     const uint8_t nonce[XPASS_NONCE_LEN]);

int xpass_vault_load(const char *path,
                     uint8_t **ciphertext, uint32_t *ciphertext_len,
                     uint8_t salt[XPASS_SALT_LEN],
                     uint8_t nonce[XPASS_NONCE_LEN]);

void xpass_secure_zero(void *buf, size_t len);

#endif
