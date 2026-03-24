cmake_minimum_required(VERSION 3.20)
project(xenonpass C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

add_library(xpass SHARED
    vault/kdf.c
    vault/crypto.c
    vault/io.c
    vault/mem.c
)

target_include_directories(xpass PUBLIC vault/include ${SODIUM_INCLUDE_DIRS})
target_link_libraries(xpass ${SODIUM_LIBRARIES})
target_link_directories(xpass PUBLIC ${SODIUM_LIBRARY_DIRS})

add_executable(test_vault tests/test_vault.c)
target_link_libraries(test_vault xpass ${SODIUM_LIBRARIES})
target_include_directories(test_vault PRIVATE vault/include ${SODIUM_INCLUDE_DIRS})
