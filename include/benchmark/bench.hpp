#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "../concepts.hpp"
#include "../grammar.hpp"
#include "sampled_scan_query_grammar.hpp"
#include <malloc_count.h>
#include <vector>

namespace gracli {

template<Queryable Grm>
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

template<Queryable Grm>
inline QGrammarResult<Grm> build_query_grammar(std::string &file) {

    std::chrono::steady_clock::time_point begin       = std::chrono::steady_clock::now();
    int64_t                               space_begin = malloc_count_current();

    Grammar gr = Grammar::from_file(file);

    std::chrono::steady_clock::time_point end       = std::chrono::steady_clock::now();
    int64_t                               space_end = malloc_count_current();

    auto    decode_time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t decode_space_delta = space_end - space_begin;

    begin       = std::chrono::steady_clock::now();
    space_begin = malloc_count_current();

    Grm qgr(std::move(gr));

    auto source_length = qgr.source_length();

    end       = std::chrono::steady_clock::now();
    space_end = malloc_count_current();

    auto    constr_time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t constr_space_delta = space_end - space_begin;

    return QGrammarResult<Grm>(std::move(qgr),
                               source_length,
                               decode_time,
                               constr_time,
                               decode_space_delta,
                               constr_space_delta);
}

template<>
inline QGrammarResult<std::string> build_query_grammar<std::string>(std::string &file) {

    Grammar gr = Grammar::from_file(file);

    int64_t     space_begin        = malloc_count_current();
    std::string source             = gr.reproduce();
    int64_t     space_end          = malloc_count_current();
    auto        source_length      = source.length();
    int64_t     decode_space_delta = space_end - space_begin;

    return QGrammarResult<std::string>(std::move(source), source_length, 0, 0, decode_space_delta, 0);
}

template<Queryable Grm>
void benchmark_random_access(QGrammarResult<Grm> &data, std::string &file, size_t num_queries, std::string name) {
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

template<Queryable Grm>
void benchmark_random_access(std::string &file, size_t num_queries, std::string name) {
    QGrammarResult<Grm> result = build_query_grammar<Grm>(file);

    benchmark_random_access<Grm>(result, file, num_queries, name);
}

template<gracli::Queryable Grm>
void benchmark_substring(QGrammarResult<Grm> &data,
                         std::string          file,
                         size_t               num_queries,
                         size_t               length,
                         std::string          name) {
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

template<>
void benchmark_substring(QGrammarResult<std::string> &data,
                         std::string                  file,
                         size_t                       num_queries,
                         size_t                       length,
                         std::string                  name) {
    srand(time(nullptr));

    std::string &qgr = data.gr;

    char buf[length];

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t start = (rand() % data.source_length);
        size_t end = std::min(start + length, data.source_length);
        std::copy(qgr.begin() + start,
                  qgr.begin() + end,
                  buf);
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

template<gracli::Queryable Grm>
void benchmark_substring(std::string file, size_t num_queries, size_t length, std::string name) {
    QGrammarResult<Grm> result = build_query_grammar<Grm>(file);
    benchmark_substring<Grm>(result, file, num_queries, length, name);
}

template<gracli::Queryable Grm>
void benchmark_substring_random(QGrammarResult<Grm> &data, std::string file, size_t num_queries, std::string name) {
    srand(time(nullptr));

    Grm &qgr = data.gr;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t      idx      = rand() % data.source_length;
        std::string accessed = qgr.substr(idx, rand() % (data.source_length - idx));
        c += accessed.length();
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    // so the calls are hopefully not optimized away
    if (c < 0) {
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
              << " type=substring_random"
              << " ds=" << name << " input_file=" << file_name << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " constr_time=" << data.constr_time
              << " decode_space_delta=" << data.decode_space_delta
              << " construction_space_delta=" << data.constr_space_delta << " decode_time=" << data.decode_time
              << " query_time_total=" << query_time_total << std::endl;
}

template<gracli::Queryable Grm>
void benchmark_substring_random(std::string file, size_t num_queries, std::string name) {
    QGrammarResult<Grm> result = build_query_grammar<Grm>(file);
    benchmark_substring_random(result, file, num_queries, name);
}

} // namespace gracli
