

#include "bit_reader.hpp"
#include "cmdline_parser.hpp"
#include "naive_query_grammar.hpp"
#include "sampled_scan_query_grammar.hpp"
#include "grammar_tuple_coder.hpp"
#include <array>
#include <chrono>
#include <concepts.hpp>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <grammar.hpp>
#include <iostream>
#include <ostream>

using std::istringstream;

using namespace gracli;

template<gracli::Queryable Grm>
void benchmark_random_access(std::string &file, size_t num_queries, std::string name) {
    srand(time(nullptr));

    Grammar gr = Grammar::from_file(file);
    auto    n  = gr.reproduce().length();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    Grm qgr(std::move(gr));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin    = std::chrono::steady_clock::now();
    size_t c = 0;
    for (size_t i = 0; i < num_queries; i++) {
        auto accessed = qgr.at(rand() % n);
        c += accessed;
    }
    end                   = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << name << " input_file=" << file << " input_size=" << n
              << " num_queries=" << num_queries << " constr_time=" << constr_time
              << " query_time_total=" << query_time_total;
}

template<gracli::Queryable Grm>
void benchmark_substring(std::string file, size_t num_queries, size_t length, std::string name) {
    srand(time(nullptr));

    Grammar gr = Grammar::from_file(file);
    auto    n  = gr.reproduce().length();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    Grm qgr(std::move(gr));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin    = std::chrono::steady_clock::now();
    size_t c = 0;
    for (size_t i = 0; i < num_queries; i++) {
        std::string accessed = qgr.substr(rand() % n, length);
        c += accessed.length();
    }
    end                   = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << name << " input_file=" << file << " input_size=" << n
              << " num_queries=" << num_queries << " substring_length=" << length
              << " constr_time=" << constr_time << " query_time_total=" << query_time_total;
}

template<gracli::Queryable Grm>
void benchmark_substring_random(std::string file, size_t num_queries, std::string name) {
    srand(time(nullptr));

    Grammar gr = Grammar::from_file(file);
    auto    n  = gr.reproduce().length();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    Grm qgr(std::move(gr));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin    = std::chrono::steady_clock::now();
    size_t c = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t      idx      = rand() % n;
        std::string accessed = qgr.substr(idx, rand() % (n - idx));
        c += accessed.length();
    }
    end                   = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << name << " input_file=" << file << " input_size=" << n
              << " num_queries=" << num_queries << " substring_length=" << "0"
              << " constr_time=" << constr_time << " query_time_total=" << query_time_total;
}

enum class GrammarType : uint8_t {
    Naive,
    SampledScan512,
    SampledScan6400,
    SampledScan12800,
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
    cp.add_unsigned('n',
                    "num_queries",
                    "N",
                    num_queries,
                    "Amount of benchmark queries");

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

    if (random_access) {
        switch (grammar_type) {
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
            case GrammarType::SampledScan12800: {
                benchmark_random_access<SampledScanQueryGrammar<12800>>(file, num_queries, "sampled_scan_12800");
                break;
            }
        }
    } else if (substring) {
        if (substring_length > 0) {
            switch (grammar_type) {
                case GrammarType::Naive: {
                    benchmark_substring<NaiveQueryGrammar>(file, num_queries, substring_length, "naive");
                    break;
                }
                case GrammarType::SampledScan512: {
                    benchmark_substring<SampledScanQueryGrammar<512>>(file, num_queries, substring_length, "sampled_scan_512");
                    break;
                }
                case GrammarType::SampledScan6400: {
                    benchmark_substring<SampledScanQueryGrammar<6400>>(file, num_queries, substring_length, "sampled_scan_6400");
                    break;
                }
                case GrammarType::SampledScan12800: {
                    benchmark_substring<SampledScanQueryGrammar<12800>>(file, num_queries, substring_length, "sampled_scan_6400");
                    break;
                }
            }
        } else {
            switch (grammar_type) {
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
                case GrammarType::SampledScan12800: {
                    benchmark_substring_random<SampledScanQueryGrammar<12800>>(file, num_queries, "sampled_scan_12800");
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
