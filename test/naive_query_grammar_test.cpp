#include "query_grammar_tests.hpp"
#include <naive_query_grammar.hpp>

class NaiveQGTestFixture : public QueryGrammarTestFixture<gracli::NaiveQueryGrammar> {};

TEST_P(NaiveQGTestFixture, RandomAccessTest) { test_random_access(); }

TEST_P(NaiveQGTestFixture, SubstringTest) { test_substr(); }

INSTANTIATE_TEST_SUITE_P(NaiveQGTests,
                         NaiveQGTestFixture,
                         ::testing::Values(QueryGrammarTestInput("test_data/fox.txt", "test_data/fox.txt.seq", 25)),
                         QueryGrammarTestInput("test_data/fox.txt", "test_data/fox.txt.rp", 25));
