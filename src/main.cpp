#include <cstdint>

#include "lzend/lzend.hpp"
#include <benchmark/bench.hpp>
#include <cmdline_parser.hpp>
#include <filesystem>
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

template<gracli::FromFile DS>
void query_interactive(const std::string &path) requires gracli::Substring<DS> && gracli::CharRandomAccess<DS> && gracli::SourceLength<DS> {
    if (!std::filesystem::exists(path)) {
        std::cerr << "file " << path << " does not exist" << std::endl;
        return;
    }
    DS ds = DS::from_file(path);
    std::string s;
    size_t n = ds.source_length();
    while (true) {
        std::cout << "> ";
        std::cin >> s;

        if (s == "h" || s == "help") {
            std::cout << "exit/quit => stop interactive mode" << std::endl;
            std::cout << "bounds => print the bounds of the string" << std::endl;
            std::cout << "<from>:<to> => access a substring" << std::endl;
            std::cout << "<index> => access a character" << std::endl;
            continue ;
        }

        if (s == "exit" || s == "e" || s == "quit" || s == "q") {
            return ;
        }
        
        if (s == "bounds" || s == "b") {
            std::cout << "bounds: [0, " << n - 1 << "] (string of " << n << " characters)" << std::endl;
            continue ;
        }

        size_t colon_pos = s.find(':');

        if (colon_pos == std::string::npos) {
            size_t i;
            try {
                i = std::stoull(s);
            } catch (std::invalid_argument const& e) {
                std::cout << "Invalid index" << std::endl;
                continue ;
            } catch (std::out_of_range const& e) {
                std::cout << "Invalid index" << std::endl;
                continue ;
            }

            std::cout << "s[" << i << "] = " << ds.at(i) << std::endl;
            continue ;
        }

        size_t from, to;
        try {
            if (colon_pos != 0) {
                from = std::stoul(s.substr(0, colon_pos));
            } else {
                from = 0;
            }

            if (colon_pos != s.length() - 1) {
                to = std::min(std::stoul(s.substr(colon_pos + 1)), n - 1);
            } else {
                to = n - 1;
            }
        } catch (std::invalid_argument const& e) {
            std::cout << "Invalid index" << std::endl;
            continue ;
        } catch (std::out_of_range const& e) {
            std::cout << "Invalid index" << std::endl;
            continue ;
        }

        size_t len = to - from + 1;
        // We need this on the heap because this can get too large for the stack
        auto *buf = new char[len + 1];
        buf[len] = 0;
        ds.substr(buf, from, len);
        std::cout << "s[" << (from != 0 ? std::to_string(from) : "") << ":" << (to != n - 1 ? std::to_string(to) : "") << "] = " << buf << std::endl;
        delete[] buf;
    }
}

int main(int argc, char **argv) {

    tlx::CmdlineParser cp;

    cp.set_author("Etienne Palanga");

    std::string file;
    cp.add_string('f', "file", file, "The input grammar file");

    bool interactive = false;
    cp.add_flag('i',
                "interactive",
                interactive,
                "Starts interactive mode in which interactive queries can be made using syntax <from>:<to>");

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
    cp.add_unsigned('d',
                    "data_structure",
                    "DATA_STRUCTURE",
                    type,
                    "The Access Data Structure to use. (0 = String, 1 = Naive, 2 = Sampled Scan 512, 3 = Sampled Scan "
                    "6400, 4 = Sampled Scan 25600, 5 = LzEnd)");

    if (!cp.process(argc, argv)) {
        return -1;
    }

    if (!(interactive || random_access || substring)) {
        interactive = true;
    }

    if (type > 5) {
        type = 0;
    }

    auto grammar_type = static_cast<GrammarType>(type);

    using namespace gracli;

    if (interactive) {
        switch (grammar_type) {
            case GrammarType::ReproducedString: {
                std::cerr << "Interactive mode not supported with string" << std::endl;
                break;
            }
            case GrammarType::Naive: {
                query_interactive<NaiveQueryGrammar>(file);
                break;
            }
            case GrammarType::SampledScan512: {
                query_interactive<SampledScanQueryGrammar<512>>(file);
                break;
            }
            case GrammarType::SampledScan6400: {
                query_interactive<SampledScanQueryGrammar<6400>>(file);
                break;
            }
            case GrammarType::SampledScan25600: {
                query_interactive<SampledScanQueryGrammar<25600>>(file);
                break;
            }
            case GrammarType::LzEnd: {
                query_interactive<lz::LzEnd>(file);
                break;
            }
        }
    } if (random_access) {
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
    }
}
