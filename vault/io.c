#include "xpass.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int xpass_vault_save(const char *path,
                     const uint8_t *ciphertext, uint32_t ciphertext_len,
                     const uint8_t salt[XPASS_SALT_LEN],
                     const uint8_t nonce[XPASS_NONCE_LEN]) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    xpass_header_t header;
    memcpy(header.magic, XPASS_MAGIC, XPASS_MAGIC_LEN);
    header.version = XPASS_VERSION;
    memcpy(header.salt, salt, XPASS_SALT_LEN);
    memcpy(header.nonce, nonce, XPASS_NONCE_LEN);
    header.ciphertext_len = ciphertext_len;

    if (fwrite(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (fwrite(ciphertext, 1, ciphertext_len, f) != ciphertext_len) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int xpass_vault_load(const char *path,
                     uint8_t **ciphertext, uint32_t *ciphertext_len,
                     uint8_t salt[XPASS_SALT_LEN],
                     uint8_t nonce[XPASS_NONCE_LEN]) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    xpass_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (memcmp(header.magic, XPASS_MAGIC, XPASS_MAGIC_LEN) != 0) {
        fclose(f);
        return -1;
    }

    memcpy(salt, header.salt, XPASS_SALT_LEN);
    memcpy(nonce, header.nonce, XPASS_NONCE_LEN);
    *ciphertext_len = header.ciphertext_len;

    *ciphertext = malloc(header.ciphertext_len);
    if (!*ciphertext) {
        fclose(f);
        return -1;
    }

    if (fread(*ciphertext, 1, header.ciphertext_len, f) != header.ciphertext_len) {
        free(*ciphertext);
        *ciphertext = NULL;
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}
