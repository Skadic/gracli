

#include "sampled_scan_query_grammar.hpp"
#include <cstdlib>
#include <grammar.hpp>
#include <iostream>

int main(int argc, char **argv) {
    using namespace gracli;

    coding::RawArgs args = *coding::parse_args();

    Grammar gr = Grammar::from_file(args.file);

    if (args.validate > -1) {
        srand(0);

        std::cout << "Generating Source String..." << std::endl;
        std::string source = gr.reproduce();
        size_t      n      = source.length();

        std::cout << "Building Query Grammar..." << std::endl;
        SampledScanQueryGrammar<64000> sqr(std::move(gr));

        std::cout << "Running " << args.validate << " access queries on " << args.file << std::endl;

        for (size_t i = 0; i < args.validate; i++) {
            size_t index = rand() % n;

            char expected = source.at(index);
            char accessed = sqr.at(index);
            if (expected != accessed) {
                std::cout << "Error in query number " << i << ": index " << index << std::endl;
                std::cout << "Expected: " << expected << std::endl;
                std::cout << "Accessed: " << accessed << std::endl;
                return 1;
            }
        }

        std::cout << "Running " << args.validate << " substring queries on " << args.file << std::endl;

        for (size_t i = 0; i < args.validate; i++) {
            size_t index = rand() % n;
            size_t len   = rand() % (n - index);

            std::string expected = source.substr(index, len);
            std::string accessed = sqr.substr(index, len);
            if (expected != accessed) {
                std::cout << "Error in query number " << i << ": index " << index << ", length " << len << std::endl;
                std::cout << "Expected: " << expected << std::endl;
                std::cout << "Accessed: " << accessed << std::endl;
                return 1;
            }
        }

        return 0;
    }

    if (args.decode) {
      std::cout << gr.reproduce() << std::endl;
      return 0;
    }
}
