#include <map>
#include <string>

#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-patterns/Patterns.hpp>

struct MacroArgs {
    std::string
        strName, strValue;
};

int
main() {
    std::map<std::string, std::string>
        mapMacros;
    size_t
        uLineCount  = 1;
    MacroArgs
        argsCmdDefine;

    auto
        fnDefineHandler =
            [&argsCmdDefine, &mapMacros] (io::IStream&, const std::optional<patt::Match>& optMatch) {
                if (!optMatch)
                    return;

                mapMacros.emplace(
                    std::move(argsCmdDefine.strName),
                    std::move(argsCmdDefine.strValue));
            };
    auto
        fnLineCounter   =
            [&uLineCount] (io::IStream&, const std::optional<patt::Match>& optMatch) {
                if (optMatch)
                    uLineCount += 1;
            };

    patt::Pattern
        ptEndl          = (patt::Str("\n") |= patt::None()),
        ptSpacing       = patt::Space() % 1,
        ptIdentifier    =
            (patt::Alpha() |= patt::Str("_")) >>
            (patt::Alnum() |= patt::Str("_")) % 0;
    patt::Pattern
        ptCmdDefine     =
            patt::Str("#define") >> ptSpacing >>
            ptIdentifier / argsCmdDefine.strName >> ptSpacing >>
            (-ptEndl >> patt::Any()) % 1 / argsCmdDefine.strValue >>
            ptEndl / fnLineCounter;
    
    io::IFileStream
        fileSource("./assets/input.txt");

    while (!fileSource.EndOfStream()) {
        std::optional<patt::Match>
            optMatch    = patt::Eval(
                            ptCmdDefine / fnDefineHandler,
                            fileSource);
        if (!optMatch) {
            io::cerr.fmt("line {}: failed to match #define command\n", uLineCount);
            break;
        }

        std::string
            strMatch    = optMatch->GetString(fileSource);
        if (strMatch.ends_with('\n'))
            strMatch.pop_back();

        io::cout.fmt("matched \"{}\"\n", strMatch);
    }

    for (const auto& [strKey, strValue] : mapMacros) {
        io::cout.fmt("{}:\t{}\n", strKey, strValue);
    }
}