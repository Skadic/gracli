#include <filesystem>
#include <iostream>
#include <grammar.hpp>

auto main(int argc, char **argv) -> int {
    using namespace gracli;

    if (argc != 2) {
        std::cerr << "Please input a grammar file" << std::endl;
        return 1;
    }

    std::string file = argv[1];

    if (std::filesystem::is_directory(file)) {
        for (const auto& file : std::filesystem::directory_iterator(file)) {
            if (file.is_directory()) {
                continue;
            }
            char *args[2] = {nullptr, const_cast<char*>(static_cast<const char*>(file.path().c_str()))};
            main(2, args);
        }
        return 0;
    }

    std::string file_name;
    auto        last = file.find_last_of('/');
    if (last < std::string::npos) {
        file_name = file.substr(last + 1);
    } else {
        file_name = file;
    }


    const auto grm = Grammar::from_file(file);

    auto num_rules                  = grm.rule_count();
    auto size                       = grm.grammar_size();
    auto [source_len, avg_rule_len] = grm.source_and_avg_rule_length();
    auto [depth, avg_depth]         = grm.max_and_avg_rule_depth();

    std::cout << std::fixed << "RESULT file=" << file_name << " num_rules=" << num_rules << " size=" << size << " source_length=" << source_len
              << " avg_rule_len=" << avg_rule_len << " depth=" << depth << " avg_depth=" << avg_depth << std::endl;
}
