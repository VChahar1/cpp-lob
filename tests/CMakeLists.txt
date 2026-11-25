add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/catch2
)

add_executable(lob_tests
    test_main.cpp
    test_order.cpp
    test_events.cpp
    test_book.cpp
    test_matching.cpp
    test_edge_cases.cpp
    test_io.cpp 
)
target_link_libraries(lob_tests PRIVATE lob catch2)

add_test(NAME lob_tests COMMAND lob_tests)