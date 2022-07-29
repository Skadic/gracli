#pragma once

#include <concepts.hpp>
#include <grammar.hpp>
#include <gtest/gtest.h>

struct QueryGrammarTestInput {
  public:
    std::string path;
    size_t      len;

    QueryGrammarTestInput(std::string path, size_t len) : path{path}, len{len} {}
    QueryGrammarTestInput(std::string path) : path{path}, len{1} {}
};

template<gracli::Queryable Grm>
class QueryGrammarTestFixture : public testing::TestWithParam<QueryGrammarTestInput> {
  public:
    void test_random_access() {
        using namespace gracli;
        QueryGrammarTestInput in   = GetParam();
        std::string           path = in.path;

        Grammar gr = Grammar::from_file(path);

        srand(0);

        std::cout << "Generating Source String..." << std::endl;
        std::string source = gr.reproduce();
        size_t      n      = source.length();

        std::cout << "Building Query Grammar..." << std::endl;
        Grm sqr(std::move(gr));

        // TODO Add a check whether the source string is generated or also read from a file.
        std::cout << "Extracting all characters in " << path
                  << " and checking them against the string generated by the grammar" << std::endl;
        for (size_t i = 0; i < n; i++) {
            auto expected = source.at(i);
            auto accessed = sqr.at(i);
            ASSERT_EQ(expected, accessed) << "Error in query at index " << i;
        }
    }

    void test_substr() {

        using namespace gracli;
        QueryGrammarTestInput in   = GetParam();
        std::string           path = in.path;
        size_t                len  = in.len;

        Grammar gr = Grammar::from_file(path);

        srand(0);

        std::cout << "Generating Source String..." << std::endl;
        std::string source = gr.reproduce();
        size_t      n      = source.length();

        std::cout << "Building Query Grammar..." << std::endl;
        Grm sqr(std::move(gr));

        // TODO Add a check whether the source string is generated or also read from a file.
        std::cout << "Extracting all " << len << "-character long substrings from " << path
                  << " and checking them against the string generated by the grammar" << std::endl;

        for (size_t i = 0; i < n - len; i++) {
            std::string expected = source.substr(i, len);
            std::string accessed = sqr.substr(i, len);
            ASSERT_EQ(expected, accessed) << "Error in query at index " << i << ", length ";
        }
    }
};
