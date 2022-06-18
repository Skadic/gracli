

#include <Grammar.hpp>

int main(int argc, char **argv) {
    using namespace gracli;

    grammar_coding::RawArgs args = *grammar_coding::parse_args();

    std::cout << "Decode? " << args.decode << std::endl;
    std::cout << "Interactive? " << args.interactive << std::endl;
    std::cout << "File? " << args.file << std::endl;
  
    std::cout << "The file is: " << args.file << std::endl;

    Grammar gr = Grammar::from_file(args.file);
    std::cout << "Grammar:" << std::endl;
    gr.print();
    std::cout << "\nSource String:\n" << gr.reproduce() << std::endl;
}
