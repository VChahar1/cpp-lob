# Catch2 is header-only (v2.x). Create an INTERFACE target so tests can link it.
add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/catch2
)

# Single test executable. Easier than separate executables per file.
add_executable(lob_tests
    test_main.cpp
    test_order.cpp
    test_events.cpp
    test_book.cpp
    test_matching.cpp
)
target_link_libraries(lob_tests PRIVATE lob catch2)

# Register with CTest so we can run via `ctest`.
add_test(NAME lob_tests COMMAND lob_tests)