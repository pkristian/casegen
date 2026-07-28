cmake_minimum_required(VERSION 3.16)

project(casegen
        VERSION 0.1.0
        DESCRIPTION "Case converter and template renderer"
        LANGUAGES C)

# C11 for _Noreturn and static_assert. Extensions stay ON, which is the default and is
# load-bearing here: getline and strdup are POSIX, and plain -std=c11 hides both.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The database editors index from. CLion builds its own model from this file directly,
# but clangd, nvim and VS Code read the JSON, and emitting it costs nothing.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CONFIGURE_DEPENDS keeps the property the hand-written Makefile had: adding a source
# needs no edit anywhere. The standard warning against file(GLOB) is about builds that
# silently miss new files — that is exactly what CONFIGURE_DEPENDS fixes, by re-globbing
# at build time. It costs one stat sweep per build, which at nine files is nothing.
file(GLOB CASEGEN_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(casegen ${CASEGEN_SOURCES})

target_compile_options(casegen PRIVATE -Wall -Wextra)
target_link_options(casegen PRIVATE -static)

# tests/runTests.sh looks for the binary at the repo root and puts that directory on
# PATH, so put it there rather than teaching every test where a build tree lives.
set_target_properties(casegen PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# GNUInstallDirs rather than a hardcoded bin/, so packagers get the lib/lib64 and
# multiarch conventions of whatever distribution they are on for free.
include(GNUInstallDirs)
install(TARGETS casegen RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
