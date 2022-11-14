googletest = cmake.subproject('googletest')
gtest = googletest.dependency('gtest')
gmock = googletest.dependency('gmock')
gtest_main = googletest.dependency('gtest_main')
gmock_main = googletest.dependency('gmock_main')

subdir('test_data')

message(googletest.target_list())

test_deps = [gtest, gmock, deps, gtest_main, gmock_main]
test_includes = [includes, include_directories('.')]

test('LzEnd tests', executable('lzend_test',
  'lzend_test.cpp',
  include_directories: test_includes,
  dependencies: test_deps
))

test('Sampled Scan Query Grammar Tests', executable('sampled_scan_query_grammar_test',
  'sampled_query_grammar_test.cpp',
  include_directories: test_includes,
  dependencies: test_deps
))


test('Naive Query Grammar Tests', executable('naive_query_grammar_test',
  'naive_query_grammar_test.cpp',
  include_directories: test_includes,
  dependencies: test_deps
))
