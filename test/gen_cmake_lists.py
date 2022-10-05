import os

executable = ""
gtest_discover = ""

files = os.listdir('.')
files.sort()

for file_name in files:
    split = os.path.splitext(file_name)
    if split[1] != ".cpp":
        continue
    executable += f"add_executable({split[0]} {file_name})\n"
    gtest_discover += f"gtest_discover_tests({split[0]} WORKING_DIRECTORY " + "${CMAKE_CURRENT_SOURCE_DIR})\n"


output = f"""cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(gracli_tests)

file(COPY test_data DESTINATION .)

# Tests
enable_testing()

link_libraries(libgracli GTest::gtest_main)

# Add executables
{executable}
include(GoogleTest)

# Discover Tests
{gtest_discover}"""

with open("./CMakeLists.txt", 'w') as out_file:
    out_file.write(output)
