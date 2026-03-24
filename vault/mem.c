#include "xpass.h"

int xpass_init(void) {
    return sodium_init();
}

void xpass_secure_zero(void *buf, size_t len) {
    sodium_memzero(buf, len);
}
