#pragma once
#include <my-iostreams/TextIO.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/ConsoleStreams.hpp>

inline int
TestTextIO() {
    std::string
        strName;
    std::string
        strAge;

    io::IOBufferStream
        buffTest;

    io::TextIO(buffTest)
        .put_str("Jhon 12")
        .go_start()
        .get(strName)
        .get(strAge);

    io::cout.fmt("{}\t{}\n", strName, strAge);

    return EXIT_SUCCESS;
}