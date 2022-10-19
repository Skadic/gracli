
#include "query_grammar_tests.hpp"
#include <sampled_scan_query_grammar.hpp>

class SampledScanQGTestFixture : public QueryGrammarTestFixture<gracli::SampledScanQueryGrammar<512>> {};

TEST_P(SampledScanQGTestFixture, RandomAccessTest) { test_random_access(); }

TEST_P(SampledScanQGTestFixture, SubstringTest) { test_substr(); }

INSTANTIATE_TEST_SUITE_P(SampledScanQGTests,
                         SampledScanQGTestFixture,
                         ::testing::Values(QueryGrammarTestInput("test_data/fox.txt", "test_data/fox.txt.seq", 25)),
                         QueryGrammarTestInput("test_data/fox.txt", "test_data/fox.txt.rp", 25));
