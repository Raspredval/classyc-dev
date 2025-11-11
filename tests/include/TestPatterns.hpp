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

static void
CaptureName(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);
        s->strMacroName     = optMatch->GetString(is);
    }
    else
        throw std::runtime_error("expected an identifier");
}

static void
CaptureValue(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);
        s->strMacroValue    = optMatch->GetString(is);
    }
    else
        throw std::runtime_error("failed to parse macro value");
}

static void
HandleMacro(io::IStream&, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);

        // TODO: expand macro here
        // io::IOBufferStream
        //    buff;
        // optMatch->Forward(is, buff);
        io::cout.fmt("found macro ${}\n", s->strMacroName);
    }
}

const patt::Pattern
    ptMacro         = (patt::Str("$") >> ptIdentifier / CaptureName) / HandleMacro;

static void
HandleDefine(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);

        std::string
            strMatch    = optMatch->GetString(is);
        if (strMatch.ends_with('\n'))
            strMatch.pop_back();
        io::cout.fmt("matched: {}\n", strMatch);

        if (s->mapGlobalMacros.contains(s->strMacroName)) {
            throw std::runtime_error(
                std::format("${} macro is already defined", s->strMacroName));
        }

        s->mapGlobalMacros.emplace(
            std::move(s->strMacroName),
            std::move(s->strMacroValue));
    }
}

const patt::Pattern
    ptCmdDefine     = (
        patt::Str("define") >> ptSpacing >>
        ptIdentifier / CaptureName >> ptSpacing >>
        ((-ptEndOfLine >> patt::Any()) % 1) / CaptureValue >>
        ptEndOfLine) / HandleDefine;

static void
HandleUndef(io::IStream& is, const std::optional<patt::Match>& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);

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
}

const patt::Pattern
    ptCmdUndef      = (
        patt::Str("undef") >> ptSpacing >>
        ptIdentifier / CaptureName >>
        ptEndOfLine) / HandleUndef;

static void
HandleCommands(io::IStream&, const std::optional<patt::Match>& optMatch, const std::any&)  {
    if (!optMatch) {
        throw std::runtime_error("unrecognized macro command");
    }
};

const patt::Pattern
    ptMacroCommand      = 
        patt::Str("#") >> (
            ptCmdDefine |=
            ptCmdUndef
        ) / HandleCommands;

const patt::Pattern
    ptSourceText        =
        (-patt::Set("#$\"\'") >> -patt::Str("//") >> patt::Any()) % 1;
const patt::Pattern
    ptPreprocessor      = (
        ptSourceText |= ptMacroCommand |= ptMacro |= ptStringLiteral |= ptCharLiteral |= ptLineComment
    ) % 0 >> patt::None();

inline int
TestPatterns() {
    io::cout.put("Testing Patterns:\n");

    io::IFileStream
        fileInput("./assets/input.txt");

    PPState
        state   = {};
    try {
        auto
            optMatch    = ptPreprocessor->Eval(fileInput, &state);
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