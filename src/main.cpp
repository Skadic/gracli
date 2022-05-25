

#include <Grammar.hpp>

int main(int argc, char **argv) {
    using namespace gracli;

    if (argc <= 1) {
        std::cout << "No Input File" << std::endl;
        exit(1);
    }

    Grammar gr = Grammar::from_file(argv[1]);
    std::cout << "Grammar:" << std::endl;
    gr.print();
    std::cout << "\nSource String:\n" << gr.reproduce() << std::endl;
}
