
#include "query_grammar_tests.hpp"
#include <sampled_scan_query_grammar.hpp>

class SampledScanQGTestFixture : public QueryGrammarTestFixture<gracli::SampledScanQueryGrammar<6400>> {};

TEST_P(SampledScanQGTestFixture, RandomAccessTest) {
    test_random_access();
}

TEST_P(SampledScanQGTestFixture, SubstringTest) {
    test_substr();
}

INSTANTIATE_TEST_SUITE_P(SampledScanQGTests,
                         SampledScanQGTestFixture,
                         ::testing::Values(QueryGrammarTestInput("dblp.seq",
                                                     25) // TestInput("dblp.seq", 25), TestInput( "dna.seq", 25 )
                                           ));
