cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(gracli)

# set C++ build flags
set(CXX_STANDARD c++20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++20 ${GCC_WARNINGS} -ldl -fopenmp")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")

set(OMP_CANCELLATION ON)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
# Copy compile_commands into the root folder
add_custom_target(
  copy-compile-commands ALL
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_LIST_DIR}/compile_commands.json
  COMMAND ${CMAKE_COMMAND} -E echo "Copied compile_commands.json into root folder"
)

## If ninja is used, this enables colored output for the build output
option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")


# -- library --
add_library(libgracli INTERFACE)

# Some compile options
target_compile_options(libgracli INTERFACE -Werror=return-type)

# include dir
target_include_directories(libgracli INTERFACE include)

# -- Dependencies --
# BitMagic
add_subdirectory(external/BitMagic EXCLUDE_FROM_ALL)
target_include_directories(libgracli INTERFACE external/BitMagic/src)

# lz-end-toolkit in-ram-parser
target_include_directories(libgracli INTERFACE external/lz-end-toolkit/extras/in-ram-parser/include)
add_library(lzend external/lz-end-toolkit/extras/in-ram-parser/src/utils.cpp)
target_link_libraries(libgracli INTERFACE lzend)

# MininalistBlockTrees
add_subdirectory(external/MinimalistBlockTrees EXCLUDE_FROM_ALL)
target_include_directories(libgracli INTERFACE external/MinimalistBlockTrees/includes)
target_link_libraries(libgracli INTERFACE blocktree)
set_target_properties(blocktree PROPERTIES COMPILE_FLAGS "-w")

# oocmd
add_subdirectory(external/oocmd)
target_include_directories(libgracli INTERFACE external/oocmd/include)

# progressbar
target_include_directories(libgracli INTERFACE external/progressbar/include)

# tlx
#add_subdirectory(external/tlx)
#target_include_directories(libgracli INTERFACE external/tlx/tlx)
#target_link_libraries(libgracli INTERFACE tlx)

# word-packing
add_subdirectory(external/word-packing)
target_include_directories(libgracli INTERFACE external/word-packing/include)

# These are only needed if this is standalone
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # malloc_count
  include(ExternalProject)
  ExternalProject_Add(malloc_count
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/malloc_count
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/malloc_count
    BUILD_COMMAND gcc -c -fpic -ldl ${CMAKE_CURRENT_SOURCE_DIR}/external/malloc_count/malloc_count.c
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
  )

  add_library(libmalloc_count INTERFACE)
  target_include_directories(libmalloc_count INTERFACE external/malloc_count)
  target_link_libraries(libmalloc_count INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/external/malloc_count/src/malloc_count-build/malloc_count.o)

  # -- executables --
  add_executable(gracli src/main.cpp)
  target_link_libraries(gracli libgracli)
  target_link_libraries(gracli libmalloc_count)

  add_executable(grammar_stats src/grammar_stats.cpp)
  target_link_libraries(grammar_stats libgracli)

  # -- tests --
  # googletest
  add_subdirectory(external/googletest)
  add_subdirectory(test EXCLUDE_FROM_ALL)
endif()