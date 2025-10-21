#include "TestPatterns.hpp"
#include "TestTextIO.hpp"

int
main(int argc, const char* argv[]) {
    if (argc != 2) {
        io::cerr.fmt("invalid arg count, expected 1, got {}", argc - 1);
        return EXIT_FAILURE;
    }

    std::string_view
        strvTest    = argv[1];
    if (strvTest == "--patterns")
        TestPatterns();
    else if (strvTest == "--text-io")
        TestTextIO();
    else {
        io::cerr.fmt("unrecognized test: {}", strvTest);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}