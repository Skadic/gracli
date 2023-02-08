#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

#include <blocktree/blocktree.hpp>
#include <concepts.hpp>
#include <file_access/file_access.hpp>
#include <grammar/grammar.hpp>
#include <grammar/sampled_scan_query_grammar.hpp>
#include <lzend/lzend.hpp>

#include <compute_lzend.hpp>
#include <malloc_count.h>

namespace gracli {

template<typename DS>
struct QueryDSResult {
    DS      ds;
    size_t  source_length;
    size_t  constr_time;
    int64_t space;

    QueryDSResult(DS &&ds, size_t source_length, size_t constr_time, int64_t space) :
        ds{std::move(ds)},
        source_length{source_length},
        constr_time{constr_time},
        space{space} {}
};

template<typename Grm>
auto build_random_access(const std::string &file) -> QueryDSResult<Grm> {
    using TimePoint = std::chrono::steady_clock::time_point;

    TimePoint begin       = std::chrono::steady_clock::now();
    size_t    space_begin = malloc_count_current();

    Grammar gr = Grammar::from_file(file);

    begin       = std::chrono::steady_clock::now();
    space_begin = malloc_count_current();

    Grm qgr(std::move(gr));

    auto source_length = qgr.source_length();

    TimePoint end       = std::chrono::steady_clock::now();
    size_t    space_end = malloc_count_current();

    size_t  constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t space       = (int64_t) space_end - (int64_t) space_begin;

    return {std::move(qgr), source_length, constr_time, space};
}

template<>
auto build_random_access<std::string>(const std::string &file) -> QueryDSResult<std::string> {
    using TimePoint = std::chrono::steady_clock::time_point;

    TimePoint   begin       = std::chrono::steady_clock::now();
    size_t      space_begin = malloc_count_current();
    std::string source;
    {
        std::ifstream     ifs(file);
        std::stringstream ss;
        ss << ifs.rdbuf();
        source = ss.str();
    }
    TimePoint end           = std::chrono::steady_clock::now();
    size_t    space_end     = malloc_count_current();
    auto      source_length = source.length();

    size_t  constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t space_delta = (int64_t) space_end - (int64_t) space_begin;

    return {std::move(source), source_length, constr_time, space_delta};
}

template<>
auto build_random_access<lz::LzEnd>(const std::string &file) -> QueryDSResult<lz::LzEnd> {
    using TimePoint = std::chrono::steady_clock::time_point;
    using namespace lz;

    // Decoding
    size_t    space_begin      = malloc_count_current();
    TimePoint begin            = std::chrono::steady_clock::now();
    auto [parsing, input_size] = decode(file);

    // Construct DS
    lz::LzEnd lz_end = lz::LzEnd::from_parsing(std::move(parsing), input_size);
    TimePoint end    = std::chrono::steady_clock::now();

    size_t  space_end   = malloc_count_current();
    size_t  constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t space       = (int64_t) space_end - (int64_t) space_begin;

    size_t source_length = lz_end.source_length();
    return {std::move(lz_end), source_length, constr_time, space};
}

template<>
auto build_random_access<FileAccess>(const std::string &file) -> QueryDSResult<FileAccess> {
    using namespace lz;
    using TimePoint = std::chrono::steady_clock::time_point;

    // Decoding
    size_t    space_begin = malloc_count_current();
    TimePoint begin       = std::chrono::steady_clock::now();
    auto      file_access = FileAccess::from_file(file);
    TimePoint end         = std::chrono::steady_clock::now();
    size_t    space_end   = malloc_count_current();
    size_t    time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t   space_delta = (int64_t) space_end - (int64_t) space_begin;

    const size_t source_length = file_access.source_length();
    return {std::move(file_access), source_length, time, space_delta};
}

template<>
auto build_random_access<BlockTreeRandomAccess>(const std::string &file) -> QueryDSResult<BlockTreeRandomAccess> {
    using namespace lz;
    using TimePoint = std::chrono::steady_clock::time_point;

    // Decoding
    size_t    space_begin = malloc_count_current();
    TimePoint begin       = std::chrono::steady_clock::now();
    auto      bt          = BlockTreeRandomAccess::from_file(file);
    TimePoint end         = std::chrono::steady_clock::now();
    size_t    space_end   = malloc_count_current();
    size_t    time        = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    int64_t   space_delta = (int64_t) space_end - (int64_t) space_begin;

    const size_t source_length = bt.source_length();
    return {std::move(bt), source_length, time, space_delta};
}

template<CharRandomAccess Grm>
void benchmark_random_access(QueryDSResult<Grm> &&data,
                             const std::string   &file,
                             size_t               num_queries,
                             const std::string   &name) {
    std::random_device                    rd;
    std::mt19937                          gen(rd());
    std::uniform_int_distribution<size_t> rand_int(0, data.source_length - 1);

    Grm &qgr = data.ds;

    size_t c = 0;

    auto begin = std::chrono::steady_clock::now();
    for (size_t i = 0; i < num_queries; i++) {
        c += qgr.at(rand_int(gen));
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
              << " num_queries=" << num_queries << " space=" << data.space << " construction_time=" << data.constr_time
              << " query_time_total=" << query_time_total << std::endl;
}

template<CharRandomAccess Grm>
void benchmark_random_access(const std::string &file, size_t num_queries, const std::string &name) {
    QueryDSResult<Grm> result = build_random_access<Grm>(file);

    benchmark_random_access<Grm>(std::move(result), file, num_queries, name);
}

template<Substring Grm>
void benchmark_substring(QueryDSResult<Grm> &&data,
                         const std::string   &file,
                         size_t               num_queries,
                         size_t               length,
                         const std::string   &name) {
    std::random_device                    rd;
    std::mt19937                          gen(rd());
    std::uniform_int_distribution<size_t> rand_int(0, data.source_length - 1);

    Grm &qgr = data.ds;

    char buf[length];

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        qgr.substr(buf, rand_int(gen), length);
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
              << " num_queries=" << num_queries << " substring_length=" << length << " space=" << data.space
              << " construction_time=" << data.constr_time << " query_time_total=" << query_time_total << std::endl;
}

void benchmark_substring(QueryDSResult<std::string> &&data,
                         const std::string           &file,
                         size_t                       num_queries,
                         size_t                       length,
                         const std::string           &name) {
    std::random_device                    rd;
    std::mt19937                          gen(rd());
    std::uniform_int_distribution<size_t> rand_int(0, data.source_length - 1);

    std::string &qgr = data.ds;

    char buf[length];

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t start = rand_int(gen);
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
              << " num_queries=" << num_queries << " substring_length=" << length << " space=" << data.space
              << " construction_time=" << data.constr_time << " query_time_total=" << query_time_total << std::endl;
}

template<Substring Grm>
void benchmark_substring(std::string file, size_t num_queries, size_t length, std::string name) {
    QueryDSResult<Grm> result = build_random_access<Grm>(file);
    benchmark_substring<Grm>(std::move(result), file, num_queries, length, name);
}

void benchmark_substring(std::string file, size_t num_queries, size_t length, const std::string &name) {
    QueryDSResult<std::string> result = build_random_access<std::string>(file);
    benchmark_substring(std::move(result), file, num_queries, length, name);
}

} // namespace gracli
