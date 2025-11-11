#pragma once
#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-iostreams/TextIO.hpp>
#include <my-patterns/Patterns.hpp>

struct PPState {
    std::map<std::string, std::string>
        mapGlobalMacros;
    std::string
        strMacroName,
        strMacroValue;
    std::vector<std::string>
        vecMacroArgs;
};

inline int
TestPatterns() {
    io::cout.put("Testing Patterns:\n");

    const patt::Pattern
        ptSpacing       = patt::Space() % 1,
        ptOptSpacing    = patt::Space() % 0,
        ptEndOfLine     = patt::None() |= patt::Str("\n"),
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
            patt::Str("\'"),
        ptLineComment   =
            patt::Str("//") >> (-ptEndOfLine >> patt::Any()) % 0 >> ptEndOfLine;
    
    auto
        fnCaptureName   =
            [] (io::IStream& is, const patt::OptMatch& optMatch, intptr_t iUserValue) {
                if (optMatch) {
                    auto* s = (PPState*)iUserValue;
                    s->strMacroName     = optMatch->GetString(is);
                }
                else
                    throw std::runtime_error("expected an identifier");
            };
    auto
        fnCaptureValue  =
            [] (io::IStream& is, const patt::OptMatch& optMatch, intptr_t iUserValue) {
                if (optMatch) {
                    auto* s = (PPState*)iUserValue;
                    s->strMacroValue    = optMatch->GetString(is);
                }
                else
                    throw std::runtime_error("failed to parse macro value");
            };
    auto
        fnExpandMacro   =
            [] (io::IStream&, const patt::OptMatch& optMatch, intptr_t iUserValue) {
                if (optMatch) {
                    auto* s = (PPState*)iUserValue;

                    // TODO: expand macro here
                    // io::IOBufferStream
                    //    buff;
                    // optMatch->Forward(is, buff);
                    io::cout.fmt("found macro ${}\n", s->strMacroName);
                }
            };
    
    const patt::Pattern
        ptMacro         = (patt::Str("$") >> ptIdentifier / fnCaptureName) / fnExpandMacro;
    
    auto
        fnHandleDefine  =
            [] (io::IStream& is, const patt::OptMatch& optMatch, intptr_t iUserValue) {
                if (optMatch) {
                    auto* s = (PPState*)iUserValue;
                    std::string
                        strMatch    = optMatch->GetString(is);
                    if (strMatch.ends_with('\n'))
                        strMatch.pop_back();
                    io::cout.fmt("matched: {}\n", strMatch);

                    if (s->mapGlobalMacros.contains(s->strMacroName)) {
                        throw std::runtime_error(
                            std::format("cannot redefine macro {}", s->strMacroName));
                    }

                    s->mapGlobalMacros.emplace(
                        std::move(s->strMacroName),
                        std::move(s->strMacroValue));
                }
            };
    const patt::Pattern
        ptCmdDefine     = (
            patt::Str("define") >> ptSpacing >>
            ptIdentifier / fnCaptureName >> ptSpacing >>
            ((-ptEndOfLine >> patt::Any()) % 1) / fnCaptureValue >>
            ptEndOfLine) / fnHandleDefine;

    auto
        fnHandleUndef   =
            [] (io::IStream& is, const std::optional<patt::Match>& optMatch, intptr_t iUserValue) {
                if (optMatch) {
                    auto* s = (PPState*)iUserValue;
                    std::string
                        strMatch    = optMatch->GetString(is);
                    if (strMatch.ends_with('\n'))
                        strMatch.pop_back();
                    io::cout.fmt("matched: {}\n", strMatch);
                    
                    auto
                        itMacro     = s->mapGlobalMacros.find(s->strMacroName);
                    if (itMacro == s->mapGlobalMacros.end()) {
                        throw std::runtime_error(
                            std::format("cannot undef nonexisting macro {}", s->strMacroName));
                    }

                    s->mapGlobalMacros.erase(itMacro);
                }
            };
    const patt::Pattern
        ptCmdUndef      = (
            patt::Str("undef") >> ptSpacing >>
            ptIdentifier / fnCaptureName >>
            ptEndOfLine) / fnHandleUndef;

    auto
        fnHandleCommands    =
            [] (io::IStream&, const std::optional<patt::Match>& optMatch, intptr_t)  {
                if (!optMatch) {
                    throw std::runtime_error("unrecognized macro command");
                }
            };
    const patt::Pattern
        ptMacroCommand      = 
            patt::Str("#") >> (
                ptCmdDefine |=
                ptCmdUndef
            ) / fnHandleCommands;

    const patt::Pattern
        ptSourceText        =
            (-patt::Str("#") >> -patt::Str("$") >> -patt::Str("\"") >> -patt::Str("\'") >> -patt::Str("//") >> patt::Any()) % 1;
    const patt::Pattern
        ptPreprocessor      = (
            ptSourceText |= ptMacroCommand |= ptMacro |= ptStringLiteral |= ptCharLiteral |= ptLineComment
        ) % 0 >> patt::None();

    io::IFileStream
        fileInput("./assets/input.txt");

    PPState
        state   = {};
    try {
        auto
            optMatch    = patt::Eval(ptPreprocessor, fileInput, (intptr_t)&state);
        if (!optMatch)
            throw std::runtime_error("failed to parse input text");
        
        io::cout.put("Macro constants:\n");
        for (const auto& [strKey, strValue] : state.mapGlobalMacros) {
            io::cout.fmt(" > {}: {}\n", strKey, strValue);
        }
    }
    catch (const std::exception& err) {
        io::cerr.fmt("[{}] Error: {}\n", fileInput.GetPosition(), err.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}