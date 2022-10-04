cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(gracli_tests)

# Tests
enable_testing()

link_libraries(libgracli GTest::gtest_main)

# Add executables
add_executable(lzend_test lzend_test.cpp)
add_executable(naive_query_grammar_test naive_query_grammar_test.cpp)
add_executable(sampled_query_grammar_test sampled_query_grammar_test.cpp)

include(GoogleTest)

# Discover Tests
gtest_discover_tests(lzend_test)
gtest_discover_tests(naive_query_grammar_test)
gtest_discover_tests(sampled_query_grammar_test)
