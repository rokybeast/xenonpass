#include "xpass.h"
#include <string.h>

int xpass_init(void) {
    return sodium_init();
}

void xpass_secure_zero(void *buf, size_t len) {
    sodium_memzero(buf, len);
}

void *vault_memcpy(void *dest, size_t dest_size, const void *src, size_t n) {
    if (n > dest_size) {
        return NULL;
    }
    return memcpy(dest, src, n);
}
