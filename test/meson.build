googletest = cmake.subproject('googletest')
gtest = googletest.dependency('gtest')
gmock = googletest.dependency('gmock')
gtest_main = googletest.dependency('gtest_main')
gmock_main = googletest.dependency('gmock_main')

subdir('test_data')

message(googletest.target_list())

test_deps = [gtest, gmock, deps, gtest_main, gmock_main]

e = executable('lzend_test',
  'lzend_test.cpp',
  include_directories: includes,
  dependencies: test_deps
)
test('LzEnd tests', e)
