#include "entry.h"
#include <stdlib.h>
#include <string.h>

static void write_u32(uint8_t *buf, uint32_t val) {
    buf[0] = (val >> 0) & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

static uint32_t read_u32(const uint8_t *buf) {
    return (uint32_t)buf[0] |
           ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

static void write_u16(uint8_t *buf, uint16_t val) {
    buf[0] = (val >> 0) & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}

static uint16_t read_u16(const uint8_t *buf) {
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static size_t entry_serialized_size(const xpass_entry_t *e) {
    return 10 +
           strlen(e->name) +
           strlen(e->username) +
           strlen(e->password) +
           strlen(e->url) +
           strlen(e->notes);
}

int xpass_entries_serialize(const xpass_entry_list_t *list,
                            uint8_t **out, size_t *out_len) {
    size_t total = 4;
    for (uint32_t i = 0; i < list->count; i++) {
        total += entry_serialized_size(&list->entries[i]);
    }

    uint8_t *buf = malloc(total);
    if (!buf) return -1;

    size_t pos = 0;
    write_u32(buf + pos, list->count);
    pos += 4;

    for (uint32_t i = 0; i < list->count; i++) {
        const xpass_entry_t *e = &list->entries[i];
        const char *fields[] = {
            e->name, e->username, e->password, e->url, e->notes
        };

        for (int f = 0; f < 5; f++) {
            uint16_t len = (uint16_t)strlen(fields[f]);
            write_u16(buf + pos, len);
            pos += 2;
            memcpy(buf + pos, fields[f], len);
            pos += len;
        }
    }

    *out = buf;
    *out_len = total;
    return 0;
}

int xpass_entries_deserialize(const uint8_t *data, size_t data_len,
                              xpass_entry_list_t *list) {
    if (data_len < 4) return -1;

    size_t pos = 0;
    uint32_t count = read_u32(data + pos);
    pos += 4;

    if (count > XPASS_MAX_ENTRIES) return -1;

    list->count = count;

    for (uint32_t i = 0; i < count; i++) {
        xpass_entry_t *e = &list->entries[i];
        char *fields[] = {
            e->name, e->username, e->password, e->url, e->notes
        };

        for (int f = 0; f < 5; f++) {
            if (pos + 2 > data_len) return -1;
            uint16_t len = read_u16(data + pos);
            pos += 2;

            if (len >= XPASS_MAX_FIELD_LEN) return -1;
            if (pos + len > data_len) return -1;

            memcpy(fields[f], data + pos, len);
            fields[f][len] = '\0';
            pos += len;
        }
    }

    return 0;
}
