#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <benchmark/bench.hpp>
#include <blocktree/blocktree.hpp>
#include <file_access/file_access.hpp>
#include <grammar/naive_query_grammar.hpp>
#include <grammar/sampled_scan_query_grammar.hpp>
#include <lzend/lzend.hpp>

#include <oocmd.hpp>
#include <progressbar.hpp>

enum class GrammarType : uint8_t {
    ReproducedString,
    Naive,
    SampledScan512,
    SampledScan6400,
    SampledScan25600,
    LzEnd,
    FileAccess,
    BlockTree,
};

template<gracli::FromFile DS>
void verify_ds(const std::string &source_path, const std::string &compressed_path) requires gracli::Substring<DS> &&
    gracli::CharRandomAccess<DS> && gracli::SourceLength<DS> {
    if (!std::filesystem::exists(source_path)) {
        std::cerr << "file " << source_path << " does not exist" << std::endl;
        return;
    }

    if (!std::filesystem::exists(compressed_path)) {
        std::cerr << "file " << compressed_path << " does not exist" << std::endl;
        return;
    }

    std::string source;

    {
        std::ifstream ifs(source_path);
        std::noskipws(ifs);
        std::stringstream ss;
        ss << ifs.rdbuf();
        source = ss.str();
    }

    DS     ds = DS::from_file(compressed_path);
    size_t n  = source.length();

    std::cout << "Checking Random Access..." << std::endl;
    progressbar bar(100);
    bar.set_done_char("=");
#pragma omp parallel
    {
#pragma omp for
        for (size_t i = 0; i < source.length(); i++) {
            char src_char = source.at(i);
            char ds_char  = ds.at(i);
            if (src_char != ds_char) {
                std::cerr << "random access at index " << i << " failed" << std::endl;
#pragma omp cancel for
            }
            if (i % (n / 100) == 0) {
#pragma omp critical
                bar.update();
            }
        }
    }

    std::cout << "\nChecking substrings..." << std::endl;
    bar.reset();
#pragma omp parallel
    {
        char src_buf[11];
        char ds_buf[11];
        src_buf[10] = 0;
        ds_buf[10]  = 0;
#pragma omp for
        for (size_t i = 0; i < n - 10; i++) {
            source.copy(src_buf, 10, i);
            ds.substr(ds_buf, i, 10);
            if (strcmp(src_buf, ds_buf)) {
#pragma omp critical
                std::cerr << "substring at index " << i << " failed.\nexpected: \"" << src_buf << "\"\nactual: \""
                          << ds_buf << "\"" << std::endl;
#pragma omp cancel for
            }
            if (i % ((n - 10) / 100) == 0) {
#pragma omp critical
                bar.update();
            }
        }
    }
    std::cout << "\nVerification successful!" << std::endl;
}

template<gracli::FromFile DS>
void query_interactive(const std::string &path) requires gracli::Substring<DS> && gracli::CharRandomAccess<DS> &&
    gracli::SourceLength<DS> {
    if (!std::filesystem::exists(path)) {
        std::cerr << "file " << path << " does not exist" << std::endl;
        return;
    }
    DS          ds = DS::from_file(path);
    std::string s;
    size_t      n = ds.source_length();
    while (true) {
        std::cout << "> ";
        std::cin >> s;

        if (s == "h" || s == "help") {
            std::cout << "exit/quit => stop interactive mode" << std::endl;
            std::cout << "bounds => print the bounds of the string" << std::endl;
            std::cout << "<from>:<to> => access a substring" << std::endl;
            std::cout << "<index> => access a character" << std::endl;
            continue;
        }

        if (s == "exit" || s == "e" || s == "quit" || s == "q") {
            return;
        }

        if (s == "bounds" || s == "b") {
            std::cout << "bounds: [0, " << n - 1 << "] (string of " << n << " characters)" << std::endl;
            continue;
        }

        size_t colon_pos = s.find(':');

        if (colon_pos == std::string::npos) {
            size_t i;
            try {
                i = std::stoull(s);
            } catch (std::invalid_argument const &e) {
                std::cout << "Invalid index" << std::endl;
                continue;
            } catch (std::out_of_range const &e) {
                std::cout << "Invalid index" << std::endl;
                continue;
            }

            std::cout << "s[" << i << "] = " << ds.at(i) << std::endl;
            continue;
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
        } catch (std::invalid_argument const &e) {
            std::cout << "Invalid index" << std::endl;
            continue;
        } catch (std::out_of_range const &e) {
            std::cout << "Invalid index" << std::endl;
            continue;
        }

        size_t len = to - from + 1;
        // We need this on the heap because this can get too large for the stack
        auto *buf = new char[len + 1];
        buf[len]  = 0;
        ds.substr(buf, from, len);
        std::cout << "s[" << (from != 0 ? std::to_string(from) : "") << ":" << (to != n - 1 ? std::to_string(to) : "")
                  << "] = " << buf << std::endl;
        delete[] buf;
    }
}

struct Gracli : public oocmd::ConfigObject {

    std::string  file;
    std::string  src_file;
    bool         interactive      = false;
    bool         random_access    = false;
    bool         substring        = false;
    bool         verify           = false;
    unsigned int substring_length = 10;
    unsigned int num_queries      = 100;
    unsigned int type             = 0;

    Gracli() : ConfigObject("gracli", "Offers various data structures for random access on compressed sequences") {
        param('f', "file", file, "The compressed input file");
        param('S', "source_file", src_file, "The uncompressed reference file for use with -v");
        param('i',
              "interactive",
              interactive,
              "Starts interactive mode in which interactive queries can be made using syntax <from>:<to>");
        param('r',
              "random_access",
              random_access,
              "Benchmarks runtime of a Grammar's random access queries. Value is the number of queries.");

        param('s',
              "substring",
              substring,
              "Benchmarks runtime of a Grammar's substring queries. Value is the number of queries.");
        param('v',
              "verify",
              verify,
              "Verifies that the given compressed file reprocudes the same characters as a given (uncompressed) "
              "reference file.");
        param('l',
              "substring_length",
              substring_length,
              "Length of the substrings while benchmarking substring queries.");
        param('n', "num_queries", num_queries, "Amount of benchmark queries");
        param('d',
              "data_structure",
              type,
              "The Access Data Structure to use. (0 = String, 1 = Naive, 2 = Sampled Scan 512, 3 = Sampled Scan "
              "6400, 4 = Sampled Scan 25600, 5 = LzEnd, 6 = File on Disk, 7 = Block Trees)");
    }

    int run(oocmd::Application const &app) {

        if (verify && (src_file.empty() || file.empty())) {
            std::cerr << "Both -f and -S are needed to use -v" << std::endl;
            return -1;
        }

        if (!(interactive || random_access || substring || verify)) {
            interactive = true;
        }

        if (type > 7) {
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
                case GrammarType::FileAccess: {
                    query_interactive<FileAccess>(file);
                    break;
                }
                case GrammarType::BlockTree: {
                    query_interactive<BlockTreeRandomAccess>(file);
                    break;
                }
            }
        } else if (verify) {
            switch (grammar_type) {
                case GrammarType::ReproducedString: {
                    std::cerr << "Verification not supported on strings" << std::endl;
                    break;
                }
                case GrammarType::Naive: {
                    verify_ds<NaiveQueryGrammar>(src_file, file);
                    break;
                }
                case GrammarType::SampledScan512: {
                    verify_ds<SampledScanQueryGrammar<512>>(src_file, file);
                    break;
                }
                case GrammarType::SampledScan6400: {
                    verify_ds<SampledScanQueryGrammar<6400>>(src_file, file);
                    break;
                }
                case GrammarType::SampledScan25600: {
                    verify_ds<SampledScanQueryGrammar<25600>>(src_file, file);
                    break;
                }
                case GrammarType::LzEnd: {
                    verify_ds<lz::LzEnd>(src_file, file);
                    break;
                }
                case GrammarType::FileAccess: {
                    verify_ds<FileAccess>(src_file, file);
                    break;
                }
                case GrammarType::BlockTree: {
                    verify_ds<BlockTreeRandomAccess>(src_file, file);
                    break;
                }
            }
        }
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
                case GrammarType::FileAccess: {
                    benchmark_random_access<FileAccess>(file, num_queries, "file_access");
                    break;
                }
                case GrammarType::BlockTree: {
                    benchmark_random_access<BlockTreeRandomAccess>(file, num_queries, "blocktree");
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
                case GrammarType::FileAccess: {
                    benchmark_substring<FileAccess>(file, num_queries, substring_length, "file_access");
                    break;
                }
                case GrammarType::BlockTree: {
                    benchmark_substring<BlockTreeRandomAccess>(file, num_queries, substring_length, "blocktree");
                    break;
                }
            }
        }

        return 0;
    }
};

auto main(int argc, char **argv) -> int {
    Gracli gracli;
    oocmd::Application::run(gracli, argc, argv);
}
