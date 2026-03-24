#include "xpass.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, name) do { \
    if (cond) { \
        printf("  PASS: %s\n", name); \
        tests_passed++; \
    } else { \
        printf("  FAIL: %s\n", name); \
        tests_failed++; \
    } \
} while (0)

static void test_encrypt_decrypt_roundtrip(void) {
    printf("[encrypt/decrypt round-trip]\n");

    uint8_t key[XPASS_KEY_LEN];
    uint8_t salt[XPASS_SALT_LEN];
    xpass_generate_salt(salt);

    const char *password = "xenon-master-key-2024";
    xpass_derive_key(key, password, strlen(password), salt);

    const char *plaintext = "user:admin\npass:s3cret\nnotes:vault entry";
    size_t pt_len = strlen(plaintext);

    uint8_t ciphertext[pt_len + XPASS_TAG_LEN];
    unsigned long long ct_len;
    uint8_t nonce[XPASS_NONCE_LEN];

    int enc_ret = xpass_encrypt(ciphertext, &ct_len,
                                (const uint8_t *)plaintext, pt_len,
                                nonce, key);
    ASSERT(enc_ret == 0, "encryption succeeds");
    ASSERT(ct_len == pt_len + XPASS_TAG_LEN, "ciphertext length correct");

    uint8_t decrypted[pt_len];
    unsigned long long dec_len;
    int dec_ret = xpass_decrypt(decrypted, &dec_len,
                                ciphertext, ct_len,
                                nonce, key);
    ASSERT(dec_ret == 0, "decryption succeeds");
    ASSERT(dec_len == pt_len, "decrypted length matches");
    ASSERT(memcmp(decrypted, plaintext, pt_len) == 0, "decrypted data matches original");

    xpass_secure_zero(key, XPASS_KEY_LEN);
}

static void test_kdf_consistency(void) {
    printf("[kdf consistency]\n");

    uint8_t salt[XPASS_SALT_LEN];
    xpass_generate_salt(salt);

    const char *password = "consistent-derivation-test";
    uint8_t key1[XPASS_KEY_LEN];
    uint8_t key2[XPASS_KEY_LEN];

    xpass_derive_key(key1, password, strlen(password), salt);
    xpass_derive_key(key2, password, strlen(password), salt);

    ASSERT(memcmp(key1, key2, XPASS_KEY_LEN) == 0, "same password + salt produces same key");

    uint8_t salt2[XPASS_SALT_LEN];
    xpass_generate_salt(salt2);
    uint8_t key3[XPASS_KEY_LEN];
    xpass_derive_key(key3, password, strlen(password), salt2);
    ASSERT(memcmp(key1, key3, XPASS_KEY_LEN) != 0, "different salt produces different key");

    xpass_secure_zero(key1, XPASS_KEY_LEN);
    xpass_secure_zero(key2, XPASS_KEY_LEN);
    xpass_secure_zero(key3, XPASS_KEY_LEN);
}

static void test_file_io_integrity(void) {
    printf("[file i/o integrity]\n");

    uint8_t key[XPASS_KEY_LEN];
    uint8_t salt[XPASS_SALT_LEN];
    xpass_generate_salt(salt);

    const char *password = "file-io-test-password";
    xpass_derive_key(key, password, strlen(password), salt);

    const char *plaintext = "site:github.com\nuser:xenon\npass:ultraS3cure!";
    size_t pt_len = strlen(plaintext);

    uint8_t ciphertext[pt_len + XPASS_TAG_LEN];
    unsigned long long ct_len;
    uint8_t nonce[XPASS_NONCE_LEN];

    xpass_encrypt(ciphertext, &ct_len,
                  (const uint8_t *)plaintext, pt_len,
                  nonce, key);

    const char *vault_path = "/tmp/test_vault.xpass";
    int save_ret = xpass_vault_save(vault_path, ciphertext, (uint32_t)ct_len, salt, nonce);
    ASSERT(save_ret == 0, "vault save succeeds");

    uint8_t *loaded_ct = NULL;
    uint32_t loaded_ct_len;
    uint8_t loaded_salt[XPASS_SALT_LEN];
    uint8_t loaded_nonce[XPASS_NONCE_LEN];

    int load_ret = xpass_vault_load(vault_path, &loaded_ct, &loaded_ct_len,
                                    loaded_salt, loaded_nonce);
    ASSERT(load_ret == 0, "vault load succeeds");
    ASSERT(loaded_ct_len == (uint32_t)ct_len, "loaded ciphertext length matches");
    ASSERT(memcmp(loaded_ct, ciphertext, ct_len) == 0, "loaded ciphertext matches");
    ASSERT(memcmp(loaded_salt, salt, XPASS_SALT_LEN) == 0, "loaded salt matches");
    ASSERT(memcmp(loaded_nonce, nonce, XPASS_NONCE_LEN) == 0, "loaded nonce matches");

    uint8_t decrypted[pt_len];
    unsigned long long dec_len;
    int dec_ret = xpass_decrypt(decrypted, &dec_len,
                                loaded_ct, loaded_ct_len,
                                loaded_nonce, key);
    ASSERT(dec_ret == 0, "decrypt from loaded vault succeeds");
    ASSERT(memcmp(decrypted, plaintext, pt_len) == 0, "decrypted loaded data matches original");

    free(loaded_ct);
    remove(vault_path);
    xpass_secure_zero(key, XPASS_KEY_LEN);
}

static void test_wrong_password(void) {
    printf("[wrong password handling]\n");

    uint8_t salt[XPASS_SALT_LEN];
    xpass_generate_salt(salt);

    const char *correct_password = "correct-horse-battery";
    const char *wrong_password = "wrong-password-attempt";

    uint8_t correct_key[XPASS_KEY_LEN];
    uint8_t wrong_key[XPASS_KEY_LEN];

    xpass_derive_key(correct_key, correct_password, strlen(correct_password), salt);
    xpass_derive_key(wrong_key, wrong_password, strlen(wrong_password), salt);

    const char *plaintext = "secret-data-that-must-stay-safe";
    size_t pt_len = strlen(plaintext);

    uint8_t ciphertext[pt_len + XPASS_TAG_LEN];
    unsigned long long ct_len;
    uint8_t nonce[XPASS_NONCE_LEN];

    xpass_encrypt(ciphertext, &ct_len,
                  (const uint8_t *)plaintext, pt_len,
                  nonce, correct_key);

    uint8_t decrypted[pt_len];
    unsigned long long dec_len;
    int dec_ret = xpass_decrypt(decrypted, &dec_len,
                                ciphertext, ct_len,
                                nonce, wrong_key);
    ASSERT(dec_ret != 0, "decryption with wrong password fails");

    xpass_secure_zero(correct_key, XPASS_KEY_LEN);
    xpass_secure_zero(wrong_key, XPASS_KEY_LEN);
}

static void test_secure_zero(void) {
    printf("[secure memory zeroing]\n");

    uint8_t buf[64];
    memset(buf, 0xAA, sizeof(buf));
    xpass_secure_zero(buf, sizeof(buf));

    int all_zero = 1;
    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    ASSERT(all_zero, "buffer is zeroed after xpass_secure_zero");
}

int main(void) {
    if (xpass_init() < 0) {
        fprintf(stderr, "FATAL: libsodium init failed\n");
        return 1;
    }

    printf("=== xenonPass vault core tests ===\n\n");

    test_encrypt_decrypt_roundtrip();
    test_kdf_consistency();
    test_file_io_integrity();
    test_wrong_password();
    test_secure_zero();

    printf("\n=== results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
