#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef MKDIR
#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0700)
#endif
#endif

#include "xpass.h"
#include "entry.h"

#define MAX_PASSWORD_LEN 256

static void resolve_vault_path(const char *filename, char *out_path, size_t out_len) {
    if (filename[0] == '/' || filename[0] == '\\' || (strlen(filename) > 2 && filename[1] == ':')) {
        snprintf(out_path, out_len, "%s", filename);
        return;
    }

    const char *base = NULL;
    char default_base[1024];

#ifdef _WIN32
    base = getenv("LOCALAPPDATA");
    if (!base) {
        const char *userprofile = getenv("USERPROFILE");
        if (userprofile) {
            snprintf(default_base, sizeof(default_base), "%s\\AppData\\Local", userprofile);
            base = default_base;
        }
    }
#else
    const char *home = getenv("HOME");
    if (home) {
        snprintf(default_base, sizeof(default_base), "%s/.local/share", home);
        base = default_base;
    }
#endif

    if (base) {
        char dir_path[2048];
        snprintf(dir_path, sizeof(dir_path), "%s/xenonpass", base);
        MKDIR(dir_path);
        
        snprintf(out_path, out_len, "%s/%s", dir_path, filename);
        
        if (strcmp(filename, out_path) != 0) {
            fprintf(stderr, "warning: auto-resolved vault path to %s\n", out_path);
        }
    } else {
        snprintf(out_path, out_len, "%s", filename);
    }
}

static int read_password(const char *prompt, char *buf, size_t buf_len) {
    struct termios old, cur;
    fprintf(stderr, "%s", prompt);

    tcgetattr(STDIN_FILENO, &old);
    cur = old;
    cur.c_lflag &= ~(tcflag_t)ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &cur);

    if (!fgets(buf, (int)buf_len, stdin)) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        return -1;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    fprintf(stderr, "\n");

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';

    return 0;
}

static int read_line(const char *prompt, char *buf, size_t buf_len) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)buf_len, stdin)) return -1;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return 0;
}

static int vault_create(const char *path, const char *password) {
    uint8_t salt[XPASS_SALT_LEN];
    uint8_t key[XPASS_KEY_LEN];
    uint8_t nonce[XPASS_NONCE_LEN];

    xpass_generate_salt(salt);
    if (xpass_derive_key(key, password, strlen(password), salt) != 0) return -1;

    xpass_entry_list_t list;
    memset(&list, 0, sizeof(list));

    uint8_t *serialized = NULL;
    size_t serialized_len = 0;
    if (xpass_entries_serialize(&list, &serialized, &serialized_len) != 0) return -1;

    size_t ct_buf_len = serialized_len + XPASS_TAG_LEN + 1;
    uint8_t *ciphertext = malloc(ct_buf_len);
    if (!ciphertext) {
        free(serialized);
        return -1;
    }

    unsigned long long ct_len = 0;
    int ret = xpass_encrypt(ciphertext, &ct_len, serialized, serialized_len, nonce, key);
    free(serialized);

    if (ret != 0) {
        free(ciphertext);
        xpass_secure_zero(key, XPASS_KEY_LEN);
        return -1;
    }

    ret = xpass_vault_save(path, ciphertext, (uint32_t)ct_len, salt, nonce);
    free(ciphertext);
    xpass_secure_zero(key, XPASS_KEY_LEN);
    return ret;
}

static int vault_load_entries(const char *path, const char *password,
                              xpass_entry_list_t *list) {
    uint8_t *ciphertext = NULL;
    uint32_t ct_len = 0;
    uint8_t salt[XPASS_SALT_LEN];
    uint8_t nonce[XPASS_NONCE_LEN];
    uint8_t key[XPASS_KEY_LEN];

    if (xpass_vault_load(path, &ciphertext, &ct_len, salt, nonce) != 0) return -1;

    if (xpass_derive_key(key, password, strlen(password), salt) != 0) {
        free(ciphertext);
        return -1;
    }

    uint8_t *plaintext = malloc(ct_len);
    if (!plaintext) {
        free(ciphertext);
        xpass_secure_zero(key, XPASS_KEY_LEN);
        return -1;
    }

    unsigned long long pt_len = 0;
    int ret = xpass_decrypt(plaintext, &pt_len, ciphertext, ct_len, nonce, key);
    free(ciphertext);
    xpass_secure_zero(key, XPASS_KEY_LEN);

    if (ret != 0) {
        free(plaintext);
        return -1;
    }

    ret = xpass_entries_deserialize(plaintext, (size_t)pt_len, list);
    free(plaintext);
    return ret;
}

static int vault_save_entries(const char *path, const char *password,
                              const xpass_entry_list_t *list) {
    uint8_t *serialized = NULL;
    size_t serialized_len = 0;
    if (xpass_entries_serialize(list, &serialized, &serialized_len) != 0) return -1;

    uint8_t salt[XPASS_SALT_LEN];
    uint8_t key[XPASS_KEY_LEN];
    uint8_t nonce[XPASS_NONCE_LEN];

    xpass_generate_salt(salt);
    if (xpass_derive_key(key, password, strlen(password), salt) != 0) {
        free(serialized);
        return -1;
    }

    size_t ct_buf_len = serialized_len + XPASS_TAG_LEN + 1;
    uint8_t *ciphertext = malloc(ct_buf_len);
    if (!ciphertext) {
        free(serialized);
        xpass_secure_zero(key, XPASS_KEY_LEN);
        return -1;
    }

    unsigned long long ct_len = 0;
    int ret = xpass_encrypt(ciphertext, &ct_len, serialized, serialized_len, nonce, key);
    free(serialized);
    xpass_secure_zero(key, XPASS_KEY_LEN);

    if (ret != 0) {
        free(ciphertext);
        return -1;
    }

    ret = xpass_vault_save(path, ciphertext, (uint32_t)ct_len, salt, nonce);
    free(ciphertext);
    return ret;
}

static void print_usage(void) {
    fprintf(stderr, "usage: xpass <command> <vault> [args...]\n");
    fprintf(stderr, "commands:\n");
    fprintf(stderr, "  init <vault>            create a new vault\n");
    fprintf(stderr, "  add  <vault>            add an entry\n");
    fprintf(stderr, "  list <vault>            list all entries\n");
    fprintf(stderr, "  get  <vault> <name>     show an entry\n");
    fprintf(stderr, "  rm   <vault> <name>     remove an entry\n");
}

static int cmd_init(const char *path) {
    char password[MAX_PASSWORD_LEN];
    char confirm[MAX_PASSWORD_LEN];

    if (read_password("master password: ", password, sizeof(password)) != 0) return 1;
    if (read_password("confirm password: ", confirm, sizeof(confirm)) != 0) return 1;

    if (strcmp(password, confirm) != 0) {
        fprintf(stderr, "passwords do not match\n");
        xpass_secure_zero(password, sizeof(password));
        xpass_secure_zero(confirm, sizeof(confirm));
        return 1;
    }
    xpass_secure_zero(confirm, sizeof(confirm));

    if (vault_create(path, password) != 0) {
        fprintf(stderr, "failed to create vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    xpass_secure_zero(password, sizeof(password));
    printf("vault created: %s\n", path);
    return 0;
}

static int cmd_add(const char *path) {
    char password[MAX_PASSWORD_LEN];
    if (read_password("master password: ", password, sizeof(password)) != 0) return 1;

    xpass_entry_list_t list;
    memset(&list, 0, sizeof(list));

    if (vault_load_entries(path, password, &list) != 0) {
        fprintf(stderr, "failed to unlock vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    if (list.count >= XPASS_MAX_ENTRIES) {
        fprintf(stderr, "vault is full\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    xpass_entry_t *e = &list.entries[list.count];
    memset(e, 0, sizeof(*e));

    read_line("name: ", e->name, sizeof(e->name));
    read_line("username: ", e->username, sizeof(e->username));
    read_line("password: ", e->password, sizeof(e->password));
    read_line("url: ", e->url, sizeof(e->url));
    read_line("notes: ", e->notes, sizeof(e->notes));

    if (strlen(e->name) == 0) {
        fprintf(stderr, "name is required\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    list.count++;

    if (vault_save_entries(path, password, &list) != 0) {
        fprintf(stderr, "failed to save vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    xpass_secure_zero(password, sizeof(password));
    printf("entry added: %s\n", e->name);
    return 0;
}

static int cmd_list(const char *path) {
    char password[MAX_PASSWORD_LEN];
    if (read_password("master password: ", password, sizeof(password)) != 0) return 1;

    xpass_entry_list_t list;
    memset(&list, 0, sizeof(list));

    if (vault_load_entries(path, password, &list) != 0) {
        fprintf(stderr, "failed to unlock vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }
    xpass_secure_zero(password, sizeof(password));

    if (list.count == 0) {
        printf("vault is empty\n");
        return 0;
    }

    for (uint32_t i = 0; i < list.count; i++) {
        printf("[%u] %s (%s)\n", i + 1, list.entries[i].name, list.entries[i].username);
    }

    return 0;
}

static int cmd_get(const char *path, const char *name) {
    char password[MAX_PASSWORD_LEN];
    if (read_password("master password: ", password, sizeof(password)) != 0) return 1;

    xpass_entry_list_t list;
    memset(&list, 0, sizeof(list));

    if (vault_load_entries(path, password, &list) != 0) {
        fprintf(stderr, "failed to unlock vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }
    xpass_secure_zero(password, sizeof(password));

    for (uint32_t i = 0; i < list.count; i++) {
        if (strcmp(list.entries[i].name, name) == 0) {
            printf("name:     %s\n", list.entries[i].name);
            printf("username: %s\n", list.entries[i].username);
            printf("password: %s\n", list.entries[i].password);
            printf("url:      %s\n", list.entries[i].url);
            printf("notes:    %s\n", list.entries[i].notes);
            return 0;
        }
    }

    fprintf(stderr, "entry not found: %s\n", name);
    return 1;
}

static int cmd_rm(const char *path, const char *name) {
    char password[MAX_PASSWORD_LEN];
    if (read_password("master password: ", password, sizeof(password)) != 0) return 1;

    xpass_entry_list_t list;
    memset(&list, 0, sizeof(list));

    if (vault_load_entries(path, password, &list) != 0) {
        fprintf(stderr, "failed to unlock vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    int found = -1;
    for (uint32_t i = 0; i < list.count; i++) {
        if (strcmp(list.entries[i].name, name) == 0) {
            found = (int)i;
            break;
        }
    }

    if (found < 0) {
        fprintf(stderr, "entry not found: %s\n", name);
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    for (uint32_t i = (uint32_t)found; i < list.count - 1; i++) {
        list.entries[i] = list.entries[i + 1];
    }
    list.count--;

    if (vault_save_entries(path, password, &list) != 0) {
        fprintf(stderr, "failed to save vault\n");
        xpass_secure_zero(password, sizeof(password));
        return 1;
    }

    xpass_secure_zero(password, sizeof(password));
    printf("entry removed: %s\n", name);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    if (xpass_init() < 0) {
        fprintf(stderr, "failed to initialize\n");
        return 1;
    }

    const char *cmd = argv[1];
    const char *raw_vault_path = argv[2];

    char vault_path[2048];
    resolve_vault_path(raw_vault_path, vault_path, sizeof(vault_path));

    if (strcmp(cmd, "init") == 0) {
        return cmd_init(vault_path);
    } else if (strcmp(cmd, "add") == 0) {
        return cmd_add(vault_path);
    } else if (strcmp(cmd, "list") == 0) {
        return cmd_list(vault_path);
    } else if (strcmp(cmd, "get") == 0) {
        if (argc < 4) {
            fprintf(stderr, "usage: xpass get <vault> <name>\n");
            return 1;
        }
        return cmd_get(vault_path, argv[3]);
    } else if (strcmp(cmd, "rm") == 0) {
        if (argc < 4) {
            fprintf(stderr, "usage: xpass rm <vault> <name>\n");
            return 1;
        }
        return cmd_rm(vault_path, argv[3]);
    }

    print_usage();
    return 1;
}
