#include <cstdint>

#include <benchmark/bench.hpp>
#include <cmdline_parser.hpp>
#include <lzend.hpp>
#include <naive_query_grammar.hpp>
#include <sampled_scan_query_grammar.hpp>

enum class GrammarType : uint8_t {
    ReproducedString,
    Naive,
    SampledScan512,
    SampledScan6400,
    SampledScan25600,
    LzEnd,
};

int main(int argc, char **argv) {

    // gracli::lz::LzEnd l("build/test.txt");

    // auto n = 5;
    // for (int i = 0; i < l.source_length() - n + 1; i++) {
    //     char buf[20];
    //     l.substr(buf, i, n);
    //     std::cout << buf << std::endl;
    // }

    tlx::CmdlineParser cp;

    cp.set_author("Etienne Palanga");

    std::string file;
    cp.add_string('f', "file", file, "The input grammar file");

    bool decode = false;
    cp.add_flag('d',
                "decode",
                decode,
                "Decodes an encoded grammar file and outputs the resulting string to stdout (default behavior)");

    bool random_access = false;
    cp.add_flag('r',
                "random_access",
                random_access,
                "Benchmarks runtime of a Grammar's random access queries. Value is the number of queries.");

    bool substring = false;
    cp.add_flag('s',
                "substring",
                substring,
                "Benchmarks runtime of a Grammar's substring queries. Value is the number of queries.");

    unsigned int substring_length = 10;
    cp.add_unsigned('l',
                    "substring_length",
                    "LENGTH",
                    substring_length,
                    "Length of the substrings while benchmarking substring queries.");

    unsigned int num_queries = 100;
    cp.add_unsigned('n', "num_queries", "N", num_queries, "Amount of benchmark queries");

    unsigned int type = 0;
    cp.add_unsigned('a',
                    "data_structure",
                    "DATA_STRUCTURE",
                    type,
                    "The Access Data Structure to use. (0 = String, 1 = Naive, 2 = Sampled Scan 512, 3 = Sampled Scan "
                    "6400, 4 = Sampled Scan 25600, 5 = LzEnd)");

    if (!cp.process(argc, argv)) {
        return -1;
    }

    if (!(decode || random_access || substring)) {
        decode = true;
    }

    if (type > 5) {
        type = 0;
    }

    auto grammar_type = static_cast<GrammarType>(type);

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
            case GrammarType::LzEnd: {
                benchmark_random_access<lz::LzEnd>(file, num_queries, "lzend");
                break;
            }
        }
    } else if (substring) {
        switch (grammar_type) {
            case GrammarType::ReproducedString: {
                benchmark_substring(file, num_queries, substring_length, "string");
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
            case GrammarType::LzEnd: {
                benchmark_substring<lz::LzEnd>(file, num_queries, substring_length, "lzend");
                break;
            }
        }
    } else if (decode) {
        Grammar gr = Grammar::from_file(file);
        std::cout << gr.reproduce() << std::endl;
        return 0;
    }
}
