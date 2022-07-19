

#include "sampled_scan_query_grammar.hpp"
#include <cstdlib>
#include <grammar.hpp>
#include <iostream>

int main(int argc, char **argv) {
    using namespace gracli;

    coding::RawArgs args = *coding::parse_args();

    Grammar gr = Grammar::from_file(args.file);

    if (args.validate > 0) {
        srand(0);

        std::cout << "Generating Source String..." << std::endl;
        std::string source = gr.reproduce();
        size_t      n      = source.length();

        std::cout << "Building Query Grammar..." << std::endl;
        SampledScanQueryGrammar<64000> sqr(std::move(gr));

        // TODO Add a check whether the source string is generated or also read from a file. 
        std::cout << "Extracting all " << args.validate << "-character long substrings on " << args.file << " and checking them against the string generated by the grammar" << std::endl;
 
        
        for (size_t i = 0; i < n - args.validate; i++) {
            std::string expected = source.substr(i, args.validate);
            std::string accessed = sqr.substr(i, args.validate);
            if (expected != accessed) {
                std::cout << "Error in query at index " << i << ", length " << args.validate << std::endl;
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
