project('gracli', 'cpp', 
  license: 'GPL-3.0-or-later', 
  subproject_dir: 'external',
  default_options: [
    'buildtype=debug',
    'cpp_std=c++20',
    'b_ndebug=if-release'
  ]
)

compiler = meson.get_compiler('cpp')
cmake = import('cmake')
buildtype = get_option('buildtype')

message(' '.join(['Current Build Type: ', buildtype]))

# Project Arguments
add_project_arguments('-fPIC', '-std=c++20', '-ldl' , '-Werror=return-type', language : 'cpp')
if buildtype == 'release' or buildtype == 'debugoptimized'
  add_project_arguments('-march=native', language: 'cpp')
endif
if buildtype == 'debug'
  add_project_arguments('-ggdb', language : 'cpp')
endif

# Subprojects
bitmagic = cmake.subproject('BitMagic')
tlx = cmake.subproject('tlx')
wordpacking = cmake.subproject('word-packing')
malloc_count = subproject('malloc_count')
lzendtoolkit = subproject('lz-end-toolkit')

# Include directories for this project and non-cmake dependencies 
includes = [
  include_directories('include'),
  include_directories('external/progressbar/include'),
]

# Dependencies
deps = [
  bitmagic.dependency('bm'),
  tlx.dependency('tlx'),
  wordpacking.dependency('word-packing'),
  dependency('threads'),
  dependency('openmp'),
  lzendtoolkit.get_variable('inramparser_dep'),
]

libgracli = shared_library('libgracli',
  include_directories: includes,
  dependencies: deps)

executable('gracli', 'src/main.cpp', 
  include_directories: includes,
  dependencies: [deps, malloc_count.get_variable('malloc_count_dep')],
)
executable('grammar_stats', 'src/grammar_stats.cpp',
  include_directories: includes,
  dependencies: deps
)

subdir('test')
