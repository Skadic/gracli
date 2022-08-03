

#include <cstdint>

#include "cmdline_parser.hpp"
#include <benchmark/bench.hpp>
#include <naive_query_grammar.hpp>
#include <sampled_scan_query_grammar.hpp>

enum class GrammarType : uint8_t {
    ReproducedString,
    Naive,
    SampledScan512,
    SampledScan6400,
    SampledScan25600,
};

int main(int argc, char **argv) {

    tlx::CmdlineParser cp;

    cp.set_author("Etienne Palanga");

    std::string file;
    cp.add_string('f', "file", file, "The input grammar file");

    bool decode;
    cp.add_flag('d',
                "decode",
                decode,
                "Decodes an encoded grammar file and outputs the resulting string to stdout (default behavior)");

    bool random_access;
    cp.add_flag('r',
                "random_access",
                random_access,
                "Benchmarks runtime of a Grammar's random access queries. Value is the number of queries.");

    bool substring;
    cp.add_flag('s',
                "substring",
                substring,
                "Benchmarks runtime of a Grammar's substring queries. Value is the number of queries.");

    unsigned int substring_length;
    cp.add_unsigned('l',
                    "substring_length",
                    "LENGTH",
                    substring_length,
                    "Length of the substrings while benchmarking substring queries.");

    unsigned int num_queries;
    cp.add_unsigned('n', "num_queries", "N", num_queries, "Amount of benchmark queries");

    unsigned int type;
    cp.add_unsigned('g',
                    "grammar_type",
                    "TYPE",
                    type,
                    "The Grammar Access Data Structure to use. (0 = Naive, 1 = Sampled Scan 512, 2 = Sampled Scan "
                    "6400, 3 = Sampled Scan 12800)");

    if (!cp.process(argc, argv)) {
        return -1;
    }

    if (!(decode || random_access || substring)) {
        decode = true;
    }

    GrammarType grammar_type = static_cast<GrammarType>(type);

    using namespace gracli;

    if (random_access) {
        switch (grammar_type) {
            case GrammarType::ReproducedString: {
                benchmark_random_access<std::string>(file, num_queries, "string");
                break;
            }
            case GrammarType::Naive: {
                benchmark_random_access<NaiveQueryGrammar>(file, num_queries, "naive");
                break;
            }
            case GrammarType::SampledScan512: {
                benchmark_random_access<SampledScanQueryGrammar<512>>(file, num_queries, "sampled_scan_512");
                break;
            }
            case GrammarType::SampledScan6400: {
                benchmark_random_access<SampledScanQueryGrammar<6400>>(file, num_queries, "sampled_scan_6400");
                break;
            }
            case GrammarType::SampledScan25600: {
                benchmark_random_access<SampledScanQueryGrammar<25600>>(file, num_queries, "sampled_scan_25600");
                break;
            }
        }
    } else if (substring) {
        if (substring_length > 0) {
            switch (grammar_type) {
                case GrammarType::ReproducedString: {
                    benchmark_substring<std::string>(file, num_queries, substring_length, "string");
                    break;
                }
                case GrammarType::Naive: {
                    benchmark_substring<NaiveQueryGrammar>(file, num_queries, substring_length, "naive");
                    break;
                }
                case GrammarType::SampledScan512: {
                    benchmark_substring<SampledScanQueryGrammar<512>>(file,
                                                                      num_queries,
                                                                      substring_length,
                                                                      "sampled_scan_512");
                    break;
                }
                case GrammarType::SampledScan6400: {
                    benchmark_substring<SampledScanQueryGrammar<6400>>(file,
                                                                       num_queries,
                                                                       substring_length,
                                                                       "sampled_scan_6400");
                    break;
                }
                case GrammarType::SampledScan25600: {
                    benchmark_substring<SampledScanQueryGrammar<25600>>(file,
                                                                        num_queries,
                                                                        substring_length,
                                                                        "sampled_scan_25600");
                    break;
                }
            }
        } else {
            switch (grammar_type) {
                case GrammarType::ReproducedString: {
                    benchmark_substring_random<std::string>(file, substring, "string");
                    break;
                }
                case GrammarType::Naive: {
                    benchmark_substring_random<NaiveQueryGrammar>(file, substring, "naive");
                    break;
                }
                case GrammarType::SampledScan512: {
                    benchmark_substring_random<SampledScanQueryGrammar<512>>(file, num_queries, "sampled_scan_512");
                    break;
                }
                case GrammarType::SampledScan6400: {
                    benchmark_substring_random<SampledScanQueryGrammar<6400>>(file, num_queries, "sampled_scan_6400");
                    break;
                }
                case GrammarType::SampledScan25600: {
                    benchmark_substring_random<SampledScanQueryGrammar<25600>>(file, num_queries, "sampled_scan_25600");
                    break;
                }
            }
        }
    } else if (decode) {
        Grammar gr = Grammar::from_file(file);
        std::cout << gr.reproduce() << std::endl;
        return 0;
    }
}
