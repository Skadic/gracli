cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(gracli_tests)

enable_testing()

link_libraries(libgracli GTest::gtest_main)

add_executable(hello_test hello_test.cpp)

include(GoogleTest)
gtest_discover_tests(hello_test)