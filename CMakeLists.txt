cmake_minimum_required(VERSION 3.20)
project(xenonpass C)
enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

add_library(xpass SHARED
    src/vault/kdf.c
    src/vault/crypto.c
    src/vault/io.c
    src/vault/mem.c
)

target_include_directories(xpass PUBLIC src/vault/include ${SODIUM_INCLUDE_DIRS})
target_link_libraries(xpass ${SODIUM_LIBRARIES})
target_link_directories(xpass PUBLIC ${SODIUM_LIBRARY_DIRS})

add_executable(test_vault tests/test_vault.c)
target_link_libraries(test_vault xpass ${SODIUM_LIBRARIES})
target_include_directories(test_vault PRIVATE src/vault/include ${SODIUM_INCLUDE_DIRS})

add_executable(xpass-cli cli/main.c cli/entry.c)
target_link_libraries(xpass-cli xpass ${SODIUM_LIBRARIES})
target_include_directories(xpass-cli PRIVATE src/vault/include cli ${SODIUM_INCLUDE_DIRS})

add_executable(test_cli_path tests/test_cli_path.c cli/entry.c)
target_link_libraries(test_cli_path xpass ${SODIUM_LIBRARIES})
target_include_directories(test_cli_path PRIVATE src/vault/include cli ${SODIUM_INCLUDE_DIRS})

add_test(NAME test_vault COMMAND test_vault)
add_test(NAME test_cli_path COMMAND test_cli_path)
