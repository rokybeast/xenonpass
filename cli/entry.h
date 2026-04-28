#ifndef XPASS_ENTRY_H
#define XPASS_ENTRY_H

#include <stddef.h>
#include <stdint.h>

#define XPASS_MAX_ENTRIES 1024
#define XPASS_MAX_FIELD_LEN 512

typedef struct {
    char name[XPASS_MAX_FIELD_LEN];
    char username[XPASS_MAX_FIELD_LEN];
    char password[XPASS_MAX_FIELD_LEN];
    char url[XPASS_MAX_FIELD_LEN];
    char notes[XPASS_MAX_FIELD_LEN];
} xpass_entry_t;

typedef struct {
    xpass_entry_t entries[XPASS_MAX_ENTRIES];
    uint32_t count;
} xpass_entry_list_t;

int xpass_entries_serialize(const xpass_entry_list_t *list,
                            uint8_t **out, size_t *out_len);

int xpass_entries_deserialize(const uint8_t *data, size_t data_len,
                              xpass_entry_list_t *list);

#endif
