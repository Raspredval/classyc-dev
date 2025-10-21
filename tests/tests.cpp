#include "TestPatterns.hpp"
#include "TestTextIO.hpp"

int
main(int argc, const char* argv[]) {
    if (argc != 2) {
        io::cerr.fmt("invalid arg count, expected 1, got {}\n", argc - 1);
        return EXIT_FAILURE;
    }

    std::string_view
        strvTest    = argv[1];
    if (strvTest == "--patterns")
        return TestPatterns();
    else if (strvTest == "--text-io")
        return TestTextIO();
    else {
        io::cerr.fmt("unrecognized test: {}\n", strvTest);
        return EXIT_FAILURE;
    }
}