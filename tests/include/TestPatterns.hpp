#pragma once
#include "my-iostreams/IOStreams.hpp"
#include <string>
#include <vector>

#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-iostreams/TextIO.hpp>
#include <my-patterns/Patterns.hpp>


struct MacroArgs {
    std::string
        strName, strValue;
};

inline int
TestPatterns() {
    io::cout.put("Testing Patterns:\n");

    io::IOBufferStream
        buffTest;
    io::TextIO(buffTest)
        .put_str("std::chrono::duration")
        .go_start();

    patt::Grammar
        gr;
    patt::Pattern
        ptID        = (patt::Str("_") |= patt::Alpha()) >> (patt::Str("_") |= patt::Alnum()) % 1;
    auto
        fnUsrValEcho=
            [] (io::IStream&, const std::optional<patt::Match>&, intptr_t iUserValue) {
                io::cout.fmt("The user value is {}\n", iUserValue);
            };

    std::vector<std::string>
        vecNames;
    gr["name"]      = ptID / vecNames / fnUsrValEcho >> (patt::Str("::") >> gr["name"]) % -1;

    patt::Pattern
        ptFullName  = gr["name"] >> patt::None();
    auto
        optMatch    = ptFullName->Eval(buffTest, 69'420);
    if (optMatch) {
        io::cout.fmt("name parts:\n");
        for (const auto& strName : vecNames) {
            io::cout.fmt("\t{}\n", strName);
        }

        return EXIT_SUCCESS;
    }
    else {
        io::cerr.put_str("failed to parse full name\n");
        return EXIT_FAILURE;
    }
}