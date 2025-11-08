#pragma once
#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-iostreams/TextIO.hpp>
#include <my-patterns/Patterns.hpp>

inline int
TestPatterns() {
    io::cout.put("Testing Patterns:\n");

    const patt::Pattern
        ptSpacing       = patt::Space() % 1,
        ptOptSpacing    = patt::Space() % 0;
    const patt::Pattern
        ptIdentifier    =
            (patt::Alpha() |= patt::Str("_")) >>
            (patt::Alnum() |= patt::Str("_")) % 0;
    const patt::Pattern
        ptStringLiteral =
            patt::Str("\"") >>
                ((patt::Str("\\") |= -patt::Str("\"")) >> patt::Any()) % 0 >>
            patt::Str("\""),
        ptCharLiteral   =
            patt::Str("\'") >>
                (patt::Str("\\") |= -patt::Str("\'")) >> patt::Any() >>
            patt::Str("\'");

    io::IOBufferStream
        buffTest;
    io::TextOutput(buffTest)
        .put("\"some string with a new line \\n\"")
        .go_start();

    {
        auto
            optmStrLiteral  = ptStringLiteral->Eval(buffTest);
        if (!optmStrLiteral) {
            io::cerr.put("failed to match string literal\n");
            return EXIT_FAILURE;
        }

        io::cout.fmt("matched valid string literal: {}\n", optmStrLiteral->GetString(buffTest));
    }

    buffTest.ClearBuffer();
    io::TextOutput(buffTest)
        .put("\'\\n\'")
        .go_start();

    {
        auto
            optmCharLiteral = ptCharLiteral->Eval(buffTest);
        if (!optmCharLiteral) {
            io::cerr.put("failed to match char literal\n");
            return EXIT_FAILURE;
        }

        io::cout.fmt("matched valid literal: {}\n", optmCharLiteral->GetString(buffTest));
    }

    buffTest.ClearBuffer();
    io::TextOutput(buffTest)
        .put("\'\\long char literal\'")
        .go_start();

    {
        auto
            optmInvalidChar = ptCharLiteral->Eval(buffTest);
        if (optmInvalidChar) {
            io::cerr.put("matched invalid char literal\n");
            return EXIT_FAILURE;
        }

        io::cout.fmt("failed to match incorrect char literal\n");
    }

    buffTest.ClearBuffer();
    io::TextOutput(buffTest)
        .put("\'\'")
        .go_start();
    
    {
        auto
            optmEmptyChar   = ptCharLiteral->Eval(buffTest);
        if (optmEmptyChar) {
            io::cerr.put("matched empty char literal\n");
            return EXIT_FAILURE;
        }

        io::cout.fmt("failed to match empty char literal\n");
    }

    return EXIT_SUCCESS;
}