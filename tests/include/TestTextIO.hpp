#pragma once
#include <my-iostreams/TextIO.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/ConsoleStreams.hpp>

inline int
TestTextIO() {
    io::cout.put("Testing TextIO:\n");

    std::string
        strKey1;
    std::string
        strValue1;
    
    std::string
        strKey2;
    double
        fValue2;
    std::string
        strUnit2;

    std::string
        strKey3;
    double
        fValue3;
    std::string
        strUnit3;

    io::IOBufferStream
        buffReplace;
    io::TextOutput(buffReplace)
        .put("London")
        .go_start();

    io::IOBufferStream
        buffTest;

    io::TextOutput(buffTest)
        .put("City New-York\n")
        .put("Temp -10.5C\n")
        .put("A.P. 760,00mmHg\n")
        .go_start();

    io::cout
        .put(buffTest.Replace(
            5, 13, // "New-York" stream pos
            buffReplace))
        .put_endl();

    io::TextInput(buffTest)
        .get(strKey1).get_word(strValue1)
        .get(strKey2).get_float(fValue2).get_word(strUnit2)
        .get(strKey3).get_float(fValue3).get_word(strUnit3);

    io::cout.fmt("{}\t{}\n", strKey1, strValue1);
    io::cout.fmt("{}\t{}\t{}\n", strKey2, fValue2, strUnit2);
    io::cout.fmt("{}\t{}\t{}\n", strKey3, fValue3, strUnit3);

    return EXIT_SUCCESS;
}