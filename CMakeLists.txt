cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(gracli)

# set C++ build flags
set(CXX_STANDARD c++20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++20 ${GCC_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")

# executable
add_executable(gracli src/main.cpp)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# enable corrosion
add_subdirectory(external/corrosion)

# include directory
target_include_directories(gracli PUBLIC include)

# dependencies
# -- word-packing --
add_subdirectory(external/word-packing)
target_include_directories(gracli PUBLIC external/word-packing/include)

# -- grm_decoder --
corrosion_import_crate(MANIFEST_PATH external/grm_decoder/Cargo.toml)
target_link_libraries(gracli PUBLIC grm_decoder)
target_include_directories(gracli PUBLIC external/grm_decoder/include)
