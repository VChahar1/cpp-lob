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
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )

    # Treat warnings as errors in CI / when explicitly requested.
    option(LOB_WERROR "Treat warnings as errors" OFF)
    if(LOB_WERROR)
        add_compile_options(-Werror)
    endif()
endif()

# The library: header-only for now (no .cpp content yet beyond stubs).
# We use INTERFACE for now and switch to STATIC once book.cpp has content.
add_library(lob STATIC
    src/book.cpp
    src/matching.cpp
    src/io.cpp
)
add_executable(lob_cli src/main_cli.cpp)
target_link_libraries(lob_cli PRIVATE lob)
add_executable(simulated_flow examples/simulated_flow.cpp)
target_link_libraries(simulated_flow PRIVATE lob)
add_executable(bench_throughput benches/bench_throughput.cpp)
target_link_libraries(bench_throughput PRIVATE lob)
target_include_directories(lob PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Tests.
enable_testing()
add_subdirectory(tests)