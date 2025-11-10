#pragma once
#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-iostreams/TextIO.hpp>
#include <my-patterns/Patterns.hpp>

using MacroConstants =
    std::map<std::string, std::string>;

inline int
TestPatterns() {
    io::cout.put("Testing Patterns:\n");

    const patt::Pattern
        ptSpacing       = patt::Space() % 1,
        ptOptSpacing    = patt::Space() % 0;
    const patt::Pattern
        ptEndOfLine     = patt::None() |= patt::Str("\n");
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

    std::string
        strMacroName,
        strMacroValue;
    const patt::Pattern
        ptCmdDefine     =
            patt::Str("define") >> ptSpacing >>
            ptIdentifier / strMacroName >> ptSpacing >>
            ((-ptEndOfLine >> patt::Any()) % 1) / strMacroValue >> ptEndOfLine;
    const patt::Pattern
        ptCmdUndef      =
            patt::Str("undef") >> ptSpacing >>
            ptIdentifier / strMacroName >> ptEndOfLine;
    
    auto
        fnHandleDefine  =
            [&strMacroName, &strMacroValue]
            (io::IStream& is, const std::optional<patt::Match>& optMatch, intptr_t iUserValue) {
                MacroConstants*
                    lpMacroConstants    = (MacroConstants*)iUserValue;
                if (optMatch) {
                    std::string
                        strMatch    = optMatch->GetString(is);
                    if (strMatch.ends_with('\n'))
                        strMatch.pop_back();
                    io::cout.fmt("matched: {}\n", strMatch);

                    if (lpMacroConstants->contains(strMacroName)) {
                        throw std::runtime_error(
                            std::format("cannot redefine macro {}", strMacroName));
                    }

                    lpMacroConstants->emplace(
                        std::move(strMacroName),
                        std::move(strMacroValue));
                }
            };
    auto
        fnHandleUndef   =
            [&strMacroName]
            (io::IStream& is, const std::optional<patt::Match>& optMatch, intptr_t iUserValue) {
                MacroConstants*
                    lpMacroConstants    = (MacroConstants*)iUserValue;
                if (optMatch) {
                    std::string
                        strMatch    = optMatch->GetString(is);
                    if (strMatch.ends_with('\n'))
                        strMatch.pop_back();
                    io::cout.fmt("matched: {}\n", strMatch);
                    
                    auto
                        itMacro     = lpMacroConstants->find(strMacroName);
                    if (itMacro == lpMacroConstants->end()) {
                        throw std::runtime_error(
                            std::format("cannot undef nonexisting macro {}", strMacroName));
                    }

                    lpMacroConstants->erase(itMacro);
                }
            };
    auto
        fnHandleCommands    =
            [] (io::IStream&, const std::optional<patt::Match>& optMatch, intptr_t)  {
                if (!optMatch) {
                    throw std::runtime_error("unrecognized macro command");
                }
            };

    io::IOBufferStream
        buffTest;
    
    io::TextOutput(buffTest)
        .put(
            "#define NAME   Pete\n"
            "#define AGE    12\n"
            "#undef  AGE\n"
            "#define AGE    16")
        .go_start();

    MacroConstants
        mapConstants;
    try {
        while (!buffTest.EndOfStream()) {
            auto
                optMatch    = patt::Eval(
                                patt::Str("#") >> (
                                    ptCmdDefine / fnHandleDefine |=
                                    ptCmdUndef / fnHandleUndef
                                ) / fnHandleCommands,
                                buffTest, (intptr_t)&mapConstants);
            if (!optMatch)
                io::cout.put("source code (probably)\n");
        }
        
        io::cout.put("Macro constants:\n");
        for (const auto& [strKey, strValue] : mapConstants) {
            io::cout.fmt(" > {}: {}\n", strKey, strValue);
        }
    }
    catch (const std::exception& err) {
        io::cerr.fmt("Error: {}\n", err.what());
        return EXIT_FAILURE;
    }
    

    return EXIT_SUCCESS;
}