cmake_minimum_required(VERSION 3.14)
project(cpp_lob LANGUAGES CXX)

# Use C++17. Modern but widely supported.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clangd / IDE support.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if not specified.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Strict warnings -- catch problems early.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif()

# The library: header-only for now (no .cpp content yet beyond stubs).
# We use INTERFACE for now and switch to STATIC once book.cpp has content.
add_library(lob STATIC
    src/book.cpp
    src/matching.cpp
)
target_include_directories(lob PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Tests.
enable_testing()
add_subdirectory(tests)