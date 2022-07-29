

#include "grammar_coding.hpp"
#include "naive_query_grammar.hpp"
#include "sampled_scan_query_grammar.hpp"
#include <chrono>
#include <concepts.hpp>
#include <cstdlib>
#include <ctime>
#include <grammar.hpp>
#include <iostream>

using namespace gracli;

template<gracli::Queryable Grm>
void benchmark_random_access(gracli::coding::RawArgs &args) {
    srand(time(nullptr));

    Grammar gr = Grammar::from_file(args.file);
    auto    n  = gr.reproduce().length();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    Grm qgr(std::move(gr));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin    = std::chrono::steady_clock::now();
    size_t c = 0;
    for (size_t i = 0; i < args.benchmark_random_access; i++) {
        auto accessed = qgr.at(rand() % n);
        c += accessed;
    }
    end                   = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << typeid(Grm).name() << " input_file=" << args.file << " input_size=" << n
              << " num_queries=" << args.benchmark_random_access << " constr_time=" << constr_time
              << " query_time_total=" << query_time_total;
}

template<gracli::Queryable Grm>
void benchmark_substring(gracli::coding::RawArgs &args) {
    srand(time(nullptr));

    Grammar gr = Grammar::from_file(args.file);
    auto    n  = gr.reproduce().length();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    Grm qgr(std::move(gr));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin    = std::chrono::steady_clock::now();
    size_t c = 0;
    for (size_t i = 0; i < args.benchmark_substring; i++) {
        std::string accessed = qgr.substr(rand() % n, args.substring_length);
        c += accessed.length();
    }
    end                   = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << typeid(Grm).name() << " input_file=" << args.file << " input_size=" << n
              << " num_queries=" << args.benchmark_substring << " substring_length=" << args.substring_length
              << " constr_time=" << constr_time << " query_time_total=" << query_time_total;
}


template<gracli::Queryable Grm>
void benchmark_substring_random(gracli::coding::RawArgs &args) {
    srand(time(nullptr));

    Grammar gr = Grammar::from_file(args.file);
    auto    n  = gr.reproduce().length();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    Grm qgr(std::move(gr));

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin    = std::chrono::steady_clock::now();
    size_t c = 0;
    for (size_t i = 0; i < args.benchmark_substring; i++) {
        size_t      idx      = rand() % n;
        std::string accessed = qgr.substr(idx, rand() % (n - idx));
        c += accessed.length();
    }
    end                   = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << typeid(Grm).name() << " input_file=" << args.file << " input_size=" << n
              << " num_queries=" << args.benchmark_substring << " substring_length=" << args.substring_length
              << " constr_time=" << constr_time << " query_time_total=" << query_time_total;
}

int main(int argc, char **argv) {
    using namespace coding;
    RawArgs args = *coding::parse_args();

    if (args.benchmark_random_access) {
        switch (args.grammar_type) {
            case GrammarType::Naive: {
                benchmark_random_access<NaiveQueryGrammar>(args);
                break;
            }
            case GrammarType::SampledScan512: {
                benchmark_random_access<SampledScanQueryGrammar<512>>(args);
                break;
            }
            case GrammarType::SampledScan6400: {
                benchmark_random_access<SampledScanQueryGrammar<6400>>(args);
                break;
            }
            case GrammarType::SampledScan12800: {
                benchmark_random_access<SampledScanQueryGrammar<12800>>(args);
                break;
            }
        }
    } else if (args.benchmark_substring) {
        if (args.substring_length > 0) {
            switch (args.grammar_type) {
                case GrammarType::Naive: {
                    benchmark_substring<NaiveQueryGrammar>(args);
                    break;
                }
                case GrammarType::SampledScan512: {
                    benchmark_substring<SampledScanQueryGrammar<512>>(args);
                    break;
                }
                case GrammarType::SampledScan6400: {
                    benchmark_substring<SampledScanQueryGrammar<6400>>(args);
                    break;
                }
                case GrammarType::SampledScan12800: {
                    benchmark_substring<SampledScanQueryGrammar<12800>>(args);
                    break;
                }
            }
        } else {
            switch (args.grammar_type) {
                case GrammarType::Naive: {
                    benchmark_substring_random<NaiveQueryGrammar>(args);
                    break;
                }
                case GrammarType::SampledScan512: {
                    benchmark_substring_random<SampledScanQueryGrammar<512>>(args);
                    break;
                }
                case GrammarType::SampledScan6400: {
                    benchmark_substring_random<SampledScanQueryGrammar<6400>>(args);
                    break;
                }
                case GrammarType::SampledScan12800: {
                    benchmark_substring_random<SampledScanQueryGrammar<12800>>(args);
                    break;
                }
            }
        }
    } else if (args.decode) {
        Grammar gr = Grammar::from_file(args.file);
        std::cout << gr.reproduce() << std::endl;
        return 0;
    }
}
