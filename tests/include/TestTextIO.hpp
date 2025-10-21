#pragma once
#include <my-iostreams/TextIO.hpp>
#include <my-iostreams/ConsoleStreams.hpp>

inline int
TestTextIO() {
    std::string
        strName;
    std::string
        strAge;

    io::cin.get(strName).get(strAge);
    io::cout.fmt("{}\t{}\n", strName, strAge);

    return EXIT_SUCCESS;
}