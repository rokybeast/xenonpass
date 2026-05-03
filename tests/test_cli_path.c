#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MKDIR(path) (void)(path)

#define main cli_main
#include "../cli/main.c"
#undef main

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, name)                                                     \
  do {                                                                         \
    if (cond) {                                                                \
      printf("  PASS: %s\n", name);                                            \
      tests_passed++;                                                          \
    } else {                                                                   \
      printf("  FAIL: %s\n", name);                                            \
      tests_failed++;                                                          \
    }                                                                          \
  } while (0)

void test_absolute_path_unix() {
    printf("[test_absolute_path_unix]\n");
    char out[2048];
    resolve_vault_path("/tmp/myvault.xpass", out, sizeof(out));
    ASSERT(strcmp(out, "/tmp/myvault.xpass") == 0, "Absolute path (/) stays absolute");
}

void test_absolute_path_windows() {
    printf("[test_absolute_path_windows]\n");
    char out[2048];
    
    resolve_vault_path("C:\\vault.xpass", out, sizeof(out));
    ASSERT(strcmp(out, "C:\\vault.xpass") == 0, "Windows drive path stays absolute");
    
    resolve_vault_path("\\\\server\\share\\vault.xpass", out, sizeof(out));
    ASSERT(strcmp(out, "\\\\server\\share\\vault.xpass") == 0, "Windows UNC path stays absolute");
}

void test_relative_path_resolution() {
  printf("[test_relative_path_resolution]\n");
  char out[2048];

  char *old_home = getenv("HOME");
  setenv("HOME", "/mock/home", 1);

  resolve_vault_path("myvault.xpass", out, sizeof(out));

  char expected[2048];
  snprintf(expected, sizeof(expected),
           "/mock/home/.local/share/xenonpass/myvault.xpass");

  ASSERT(strcmp(out, expected) == 0,
         "Relative path resolves to user data directory");

  if (old_home) {
    setenv("HOME", old_home, 1);
  } else {
    unsetenv("HOME");
  }
}

void test_no_home_resolution() {
  printf("[test_no_home_resolution]\n");
  char out[2048];

  char *old_home = getenv("HOME");
  unsetenv("HOME");

  resolve_vault_path("myvault.xpass", out, sizeof(out));

  ASSERT(strcmp(out, "myvault.xpass") == 0,
         "Falls back to relative path if HOME is not set");

  if (old_home)
    setenv("HOME", old_home, 1);
}

int main() {
    printf("=== xenonPass CLI Path System Tests ===\n\n");
    
    test_absolute_path_unix();
    test_absolute_path_windows();
    test_relative_path_resolution();
    test_no_home_resolution();
    
    printf("\n=== results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
