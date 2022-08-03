#pragma once

#include <chrono>
#include <fstream>

#include "../concepts.hpp"
#include "../grammar.hpp"

namespace gracli {

template<Queryable Grm>
struct QGrammarResult {
    Grm    gr;
    size_t source_length;
    size_t decode_time;
    size_t constr_time;

    QGrammarResult(Grm &&gr, size_t source_length, size_t decode_time, size_t constr_time) :
        gr{std::move(gr)},
        source_length{source_length},
        decode_time{decode_time},
        constr_time{constr_time} {}
};

template<Queryable Grm>
inline QGrammarResult<Grm> build_query_grammar(std::string &file) {

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    Grammar                               gr    = Grammar::from_file(file);
    std::chrono::steady_clock::time_point end   = std::chrono::steady_clock::now();
    auto decode_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    auto n = gr.reproduce().length();

    begin = std::chrono::steady_clock::now();
    Grm qgr(std::move(gr));
    end = std::chrono::steady_clock::now();

    auto constr_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    return QGrammarResult<Grm>(std::move(qgr), n, decode_time, constr_time);
}

template<>
inline QGrammarResult<std::string> build_query_grammar<std::string>(std::string &file) {

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    // Source: https://stackoverflow.com/questions/2912520/read-file-contents-into-a-string-in-c
    std::ifstream ifs(file);
    std::string   content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    std::chrono::steady_clock::time_point end   = std::chrono::steady_clock::now();
    auto decode_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    auto source_length = content.length();

    return QGrammarResult<std::string>(std::move(content), source_length, decode_time, 0);
}

template<Queryable Grm>
void benchmark_random_access(QGrammarResult<Grm> &data, std::string &file, size_t num_queries, std::string name) {
    srand(time(nullptr));

    Grm &qgr = data.gr;

    auto   begin = std::chrono::steady_clock::now();
    size_t c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        auto accessed = qgr.at(rand() % data.source_length);
        c += accessed;
    }
    auto end              = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << name << " input_file=" << file << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " construction_time=" << data.constr_time
              << " decode_time=" << data.decode_time << " query_time_total=" << query_time_total;
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

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t                                c     = 0;
    for (size_t i = 0; i < num_queries; i++) {
        std::string accessed = qgr.substr(rand() % data.source_length, length);
        c += accessed.length();
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto query_time_total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "RESULT"
              << " ds=" << name << " input_file=" << file << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " substring_length=" << length
              << " construction_time=" << data.constr_time << " decode_time=" << data.decode_time
              << " query_time_total=" << query_time_total;
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

    std::cout << "RESULT"
              << " ds=" << name << " input_file=" << file << " input_size=" << data.source_length
              << " num_queries=" << num_queries << " substring_length="
              << "0"
              << " constr_time=" << data.constr_time << " decode_time=" << data.decode_time
              << " query_time_total=" << query_time_total;
}

template<gracli::Queryable Grm>
void benchmark_substring_random(std::string file, size_t num_queries, std::string name) {
    QGrammarResult<Grm> result = build_query_grammar<Grm>(file);
    benchmark_substring_random(result, file, num_queries, name);
}

} // namespace gracli
