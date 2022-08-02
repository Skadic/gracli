cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(gracli)

# set C++ build flags
set(CXX_STANDARD c++20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++20 ${GCC_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -lprofiler")


# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
# Copy compile_commands into the root folder instead of just sitting in the build folder
add_custom_target(
  copy-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_LIST_DIR}/compile_commands.json
  COMMAND ${CMAKE_COMMAND} -E echo "Copied compile_commands.json into root folder"
)

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Add submodules
add_subdirectory(external/word-packing)
add_subdirectory(external/googletest)
add_subdirectory(external/tlx)

# -- library --
add_library(libgracli INTERFACE)
# include dir
target_include_directories(libgracli INTERFACE include)
# word-packing
target_include_directories(libgracli INTERFACE external/word-packing/include)
# tlx
target_include_directories(libgracli INTERFACE external/tlx/tlx)
target_link_libraries(libgracli INTERFACE tlx)

# -- executable --
add_executable(gracli src/main.cpp)
target_link_libraries(gracli libgracli)

# -- tests --
add_subdirectory(test EXCLUDE_FROM_ALL)
