#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include <concepts.hpp>
#include <grammar.hpp>
#include <sampled_scan_query_grammar.hpp>

#include <lzend/lzend.hpp>
#include <compute_lzend.hpp>
#include <malloc_count.h>

namespace gracli {

template<typename Grm>
struct QGrammarResult {
    Grm     gr;
    size_t  source_length;
    size_t  decode_time;
    size_t  constr_time;
    int64_t decode_space_delta;
    int64_t constr_space_delta;

    QGrammarResult(Grm   &&gr,
                   size_t  source_length,
                   size_t  decode_time,
                   size_t  constr_time,
                   int64_t decode_space_delta,
                   int64_t constr_space_delta) :
        gr{std::move(gr)},
        source_length{source_length},
        decode_time{decode_time},
        constr_time{constr_time},
        decode_space_delta{decode_space_delta},
        constr_space_delta{constr_space_delta} {}
};

template<typename Grm>
inline QGrammarResult<Grm> build_random_access(const std::string &file) {

    std::chrono::steady_clock::time_point begin       = std::chrono::steady_clock::now();
    size_t                                space_begin = malloc_count_current();

    Grammar gr = Grammar::from_file(file);

    std::chrono::steady_clock::time_point end       = std::chrono::steady_clock::now();
    size_t                                space_end = malloc_count_current();

    size_t    decode_time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t decode_space_delta = (int64_t) space_end - (int64_t) space_begin;

    begin       = std::chrono::steady_clock::now();
    space_begin = malloc_count_current();

    Grm qgr(std::move(gr));

    auto source_length = qgr.source_length();

    end       = std::chrono::steady_clock::now();
    space_end = malloc_count_current();

    size_t    constr_time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t constr_space_delta = (int64_t) space_end - (int64_t) space_begin;

    return {std::move(qgr), source_length, decode_time, constr_time, decode_space_delta, constr_space_delta};
}

template<>
inline QGrammarResult<std::string> build_random_access<std::string>(const std::string &file) {
    Grammar gr = Grammar::from_file(file);

    size_t      space_begin        = malloc_count_current();
    std::string source             = gr.reproduce();
    size_t      space_end          = malloc_count_current();
    auto        source_length      = source.length();
    int64_t     decode_space_delta = (int64_t) space_end - (int64_t) space_begin;

    return {std::move(source), source_length, 0, 0, decode_space_delta, 0};
}

template<>
inline QGrammarResult<lz::LzEnd> build_random_access<lz::LzEnd>(const std::string &file) {
    using namespace lz;

    std::ifstream in(file, std::ios::binary);
    std::noskipws(in);
    LzEnd::Parsing                        parsing;
    size_t                                input_size;
    size_t                                space_begin;
    std::chrono::steady_clock::time_point begin;
    {
        // We want to deallocate the vector after construction
        std::vector<LzEnd::Char> input((std::istream_iterator<LzEnd::Char>(in)), std::istream_iterator<LzEnd::Char>());
        input_size  = input.size();
        space_begin = malloc_count_current();
        begin       = std::chrono::steady_clock::now();
        compute_lzend(input.data(), input.size(), &parsing);
    }

    lz::LzEnd                             lz_end    = lz::LzEnd::from_parsing(std::move(parsing), input_size);
    size_t                                space_end = malloc_count_current();
    std::chrono::steady_clock::time_point end       = std::chrono::steady_clock::now();

    size_t  constr_time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t constr_space_delta = (int64_t) space_end - (int64_t) space_begin;

    size_t source_length = lz_end.source_length();
    return {std::move(lz_end), source_length, 0, constr_time, 0, constr_space_delta};
}

template<CharRandomAccess Grm>
void benchmark_random_access(QGrammarResult<Grm> &&data,
                             const std::string    &file,
                             size_t                num_queries,
                             const std::string    &name) {
    srand(time(nullptr));

    Grm &qgr = data.gr;

    size_t c = 0;

    auto begin = std::chrono::steady_clock::now();
    for (size_t i = 0; i < num_queries; i++) {
        c += qgr.at(rand() % data.source_length);
    }
    auto end              = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    // so the calls are hopefully not optimized away
    if (c < 1) {
        std::cout << c;
    }

    std::string file_name;
    auto        last = file.find_last_of('/');
    if (last < std::string::npos) {
        file_name = file.substr(last + 1);
    } else {
        file_name = file;
    }

    std::cout << "RESULT"
              << " type=random_access"
              << " ds=" << name << " input_file=" << file_name << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " construction_time=" << data.constr_time
              << " decode_space_delta=" << data.decode_space_delta
              << " construction_space_delta=" << data.constr_space_delta << " decode_time=" << data.decode_time
              << " query_time_total=" << query_time_total << std::endl;
}

template<CharRandomAccess Grm>
void benchmark_random_access(const std::string &file, size_t num_queries, const std::string &name) {
    QGrammarResult<Grm> result = build_random_access<Grm>(file);

    benchmark_random_access<Grm>(std::move(result), file, num_queries, name);
}

template<Substring Grm>
void benchmark_substring(QGrammarResult<Grm> &&data,
                         const std::string    &file,
                         size_t                num_queries,
                         size_t                length,
                         const std::string    &name) {
    srand(time(nullptr));

    Grm &qgr = data.gr;

    char buf[length];

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        qgr.substr(buf, rand() % data.source_length, length);
        c += buf[0];
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    // so the calls are hopefully not optimized away
    if (c < 1) {
        std::cout << c;
    }

    std::string file_name;
    auto        last = file.find_last_of('/');
    if (last < std::string::npos) {
        file_name = file.substr(last + 1);
    } else {
        file_name = file;
    }

    std::cout << "RESULT"
              << " type=substring"
              << " ds=" << name << " input_file=" << file_name << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " substring_length=" << length
              << " construction_time=" << data.constr_time << " decode_time=" << data.decode_time
              << " decode_space_delta=" << data.decode_space_delta
              << " construction_space_delta=" << data.constr_space_delta << " query_time_total=" << query_time_total
              << std::endl;
}

void benchmark_substring(QGrammarResult<std::string> &&data,
                         const std::string            &file,
                         size_t                        num_queries,
                         size_t                        length,
                         const std::string            &name) {
    srand(time(nullptr));

    std::string &qgr = data.gr;

    char buf[length];

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t start = (rand() % data.source_length);
        size_t end   = std::min(start + length, data.source_length);
        std::copy(qgr.begin() + start, qgr.begin() + end, buf);
        c += buf[0];
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    // so the calls are hopefully not optimized away
    if (c < 1) {
        std::cout << c;
    }

    std::string file_name;
    auto        last = file.find_last_of('/');
    if (last < std::string::npos) {
        file_name = file.substr(last + 1);
    } else {
        file_name = file;
    }

    std::cout << "RESULT"
              << " type=substring"
              << " ds=" << name << " input_file=" << file_name << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " substring_length=" << length
              << " construction_time=" << data.constr_time << " decode_time=" << data.decode_time
              << " decode_space_delta=" << data.decode_space_delta
              << " construction_space_delta=" << data.constr_space_delta << " query_time_total=" << query_time_total
              << std::endl;
}

template<Substring Grm>
void benchmark_substring(std::string file, size_t num_queries, size_t length, std::string name) {
    QGrammarResult<Grm> result = build_random_access<Grm>(file);
    benchmark_substring<Grm>(std::move(result), file, num_queries, length, name);
}

void benchmark_substring(std::string file, size_t num_queries, size_t length, const std::string &name) {
    QGrammarResult<std::string> result = build_random_access<std::string>(file);
    benchmark_substring(std::move(result), file, num_queries, length, name);
}

} // namespace gracli
