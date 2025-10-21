#pragma once

#include <map>
#include <string>

#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-patterns/Patterns.hpp>

struct MacroArgs {
    std::string
        strName, strValue;
};

inline int
TestPatterns() {
    std::map<std::string, std::string>
        mapMacros;
    size_t
        uLineCount      = 1;
    MacroArgs
        args;
    
    auto
        fnHandleDefine  =
            [&args, &mapMacros] (io::IStream&, const std::optional<patt::Match>& optMatch) {
                if (!optMatch)
                    return;

                mapMacros.emplace(
                    std::move(args.strName),
                    std::move(args.strValue));
            };
    auto
        fnCountLines    =
            [&uLineCount] (io::IStream&, const std::optional<patt::Match>& optMatch) {
                if (optMatch)
                    uLineCount += 1;
            };

    patt::Pattern
        ptEndl          = patt::Str("\n") |= patt::None(),
        ptSpacing       = patt::Space() % 1,
        ptIdentifier    =
            (patt::Alpha() |= patt::Str("_")) >>
            (patt::Alnum() |= patt::Str("_")) % 0;
    patt::Pattern
        ptCmdDefine     =
            patt::Str("#define") >> ptSpacing >>
            ptIdentifier / args.strName >> ptSpacing >>
            (-ptEndl >> patt::Any()) % 1 / args.strValue >>
            ptEndl / fnCountLines;
    patt::Pattern
        ptCommands      =
            (ptCmdDefine / fnHandleDefine) % 0 >> patt::None();
        
    io::IFileStream
        fileSource("./assets/input.txt");

    std::optional<patt::Match>
        optMatch    = patt::Eval(
                        ptCommands,
                        fileSource);
    if (!optMatch) {
        io::cerr.fmt("line {}: failed to match\n", uLineCount);
        return EXIT_FAILURE;
    }

    for (const auto& [strKey, strValue] : mapMacros) {
        io::cout.fmt("{}:\t{}\n", strKey, strValue);
    }

    return EXIT_SUCCESS;
}