#pragma once

#include "gtest/gtest.h"
#include <concepts.hpp>
#include <filesystem>
#include <string>

#include <grammar/grammar.hpp>
#include <util/util.hpp>

#include <gtest/gtest.h>

struct QueryGrammarTestInput {
  public:
    std::filesystem::path source_path;
    std::filesystem::path compressed_path;
    size_t                len;

    QueryGrammarTestInput(const std::string source_path, const std::string compressed_path, size_t len) :
        source_path{std::filesystem::absolute(source_path)},
        compressed_path{std::filesystem::absolute(compressed_path)},
        len{len} {}

    QueryGrammarTestInput(const std::string source_path, const std::string compressed_path) :
        QueryGrammarTestInput(source_path, compressed_path, 1) {}

    void check_paths() {
        ASSERT_TRUE(std::filesystem::exists(source_path)) << "Test file " << source_path << " does not exist";
        ASSERT_TRUE(std::filesystem::exists(compressed_path)) << "Test file " << compressed_path << " does not exist";
    }

    std::string operator()(const testing::TestParamInfo<QueryGrammarTestInput> &info) {
        return std::to_string(info.index);
    }
};

template<gracli::RandomAccess Grm>
    requires gracli::FromFile<Grm>
class QueryGrammarTestFixture : public testing::TestWithParam<QueryGrammarTestInput> {
  public:
    void test_random_access() {
        using namespace gracli;
        QueryGrammarTestInput in = GetParam();
        in.check_paths();
        std::string source_path     = in.source_path;
        std::string compressed_path = in.compressed_path;

        srand(0);

        std::string source = read_to_string(source_path);
        size_t      n      = source.length();
        Grm         grm    = Grm::from_file(compressed_path);

        ASSERT_EQ(n, grm.source_length()) << "Source length in grammar does not match actual source's length";

        for (size_t i = 0; i < n; i++) {
            auto expected = source.at(i);
            auto accessed = grm.at(i);
            ASSERT_EQ(expected, accessed) << "Error in query at index " << i;
        }
    }

    void test_substr() {
        using namespace gracli;
        QueryGrammarTestInput in = GetParam();
        in.check_paths();
        std::string source_path     = in.source_path;
        std::string compressed_path = in.compressed_path;
        size_t      len             = in.len;

        srand(0);

        std::string source = read_to_string(source_path);
        size_t      n      = source.length();
        Grm         grm    = Grm::from_file(compressed_path);

        ASSERT_EQ(n, grm.source_length()) << "Source length in grammar does not match actual source's length";
        char expected_buf[len + 1];
        char accessed_buf[len + 1];

        expected_buf[len] = 0;
        accessed_buf[len] = 0;

        for (size_t i = 0; i < n - len; i++) {
            std::copy(source.begin() + i, source.begin() + i + len, expected_buf);
            grm.substr(accessed_buf, i, len);

            for (int j = 0; j < len; j++) {
                ASSERT_TRUE(strcmp(expected_buf, accessed_buf) == 0)
                    << "Error in query at index " << i << ", length " << len
                    << "\nexpected: " << std::string(expected_buf, expected_buf + len)
                    << "\nactual: " << std::string(accessed_buf, accessed_buf + len);
            }
        }
    }
};
