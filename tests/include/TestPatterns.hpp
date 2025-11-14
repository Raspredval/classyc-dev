#pragma once
#include "my-iostreams/IOStreams.hpp"
#include <any>
#include <format>
#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-iostreams/FileStreams.hpp>
#include <my-iostreams/TextIO.hpp>
#include <my-patterns/Patterns.hpp>
#include <stdexcept>

struct InputSource {
    std::string
        strDescr;
    std::shared_ptr<io::IStream>
        sptrInput;
    std::flat_set<intptr_t>
        setNewLines;

    intptr_t
    CurLine() {
        intptr_t
            iCurline    = (intptr_t)this->setNewLines.size() + 1,
            iLastLF     = (this->setNewLines.size() > 0)
                            ? *(this->setNewLines.cend() - 1)
                            : 0;
        if (this->sptrInput->GetPosition() < iLastLF)
            iCurline    -= 1;

        return iCurline;
    }

    intptr_t
    CurColumn() {
        intptr_t
            iCurColumn  = this->sptrInput->GetPosition(),
            iLastLF     = (this->setNewLines.size() > 0)
                            ? *(this->setNewLines.cend() - 1)
                            : 0;
        if (this->sptrInput->GetPosition() < iLastLF) {
            iLastLF     = (this->setNewLines.size() > 1)
                            ? *(this->setNewLines.cend() - 2)
                            : 0;
        }

        
        return iCurColumn - iLastLF + 1;
    }

    template<typename... Args>
    [[noreturn]] void
    Error(const std::format_string<Args...>& strfmt, Args&&... args) {
        std::string
            strMessage  = std::format("[{}: Ln {}, Col {}]\n > 🚫 Error: {} 🚫\n",
                            this->strDescr,
                            this->CurLine(),
                            this->CurColumn(),
                            std::format(strfmt, std::forward<Args>(args)...));
        throw std::runtime_error(strMessage);
    }

    template<typename... Args>
    void
    Warning(const std::format_string<Args...>& strfmt, Args&&... args) {
        std::string
            strMessage  = std::format("[{}: Ln {}, Col {}]\n > ⚠️ Warning: {} ⚠️\n",
                            this->strDescr,
                            this->CurLine(),
                            this->CurColumn(),
                            std::format(strfmt, std::forward<Args>(args)...));
        io::cerr.put(strMessage);
    }

    template<typename... Args>
    void
    Message(const std::format_string<Args...>& strfmt, Args&&... args) {
        std::string
            strMessage  = std::format("[{}: Ln {}, Col {}]\n > 📨 {}\n",
                            this->strDescr,
                            this->CurLine(),
                            this->CurColumn(),
                            std::format(strfmt, std::forward<Args>(args)...));
        io::cout.put(strMessage);
    }
};

struct ParseArgs {
    std::string
        strName,
        strValue;
    std::vector<std::string>
        vecArgs;
};

struct PPState {
    std::map<std::string, std::string>
        mapGlobalMacros;
    ParseArgs
        pa;
    std::vector<InputSource>
        vecSources;
};

const patt::Pattern
    ptSpacing       = patt::Space() % 1,
    ptOptSpacing    = patt::Space() % 0,
    ptIdentifier    =
        (patt::Alpha() |= patt::Str("_")) >>
        (patt::Alnum() |= patt::Str("_")) % 0;

static void
HandleNewLine(io::IStream&, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* state     = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = state->vecSources.back();
        cur_source.setNewLines.emplace(
            cur_source.sptrInput->GetPosition());
    }
};

const patt::Pattern
    ptLineFeed  = (patt::Str("\n")) / HandleNewLine,
    ptEndOfLine = patt::None() |= ptLineFeed;

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
        patt::Str("//") >> (-ptEndOfLine >> patt::Any()) % 0 >> &ptEndOfLine;

static void
CaptureName(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    auto* s = std::any_cast<PPState*>(user_data);
    if (optMatch) {
        s->pa.strName   = optMatch->GetString(is);
    }
    else {
        auto&
            cur_source  = s->vecSources.back();
        cur_source.Error("expected an identifier");
    }
}

static void
CaptureValue(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    auto* s = std::any_cast<PPState*>(user_data);
    if (optMatch) {
        s->pa.strValue  = optMatch->GetString(is);
    }
    else {
        auto&
            cur_source  = s->vecSources.back();
        cur_source.Error("failed to parse macro value");
    }
}

static void
HandleMacro(io::IStream&, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* state     = std::any_cast<PPState*>(user_data);

        // TODO: expand macro here
        // io::IOBufferStream
        //    buff;
        // optMatch->Forward(is, buff);
        auto&
            cur_source  = state->vecSources.back();
        cur_source.Message("found macro ${}", state->pa.strName);
    }
}

const patt::Pattern
    ptMacro         = (patt::Str("$") >> ptIdentifier / CaptureName) / HandleMacro;

static void
HandleDefine(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = s->vecSources.back();

        std::string
            strMatch    = optMatch->GetString(is);
        if (strMatch.ends_with('\n'))
            strMatch.pop_back();
        cur_source.Message("found command: [#{}]", strMatch);

        if (s->mapGlobalMacros.contains(s->pa.strName))
            cur_source.Error("${} macro is already defined", s->pa.strName);

        s->mapGlobalMacros.emplace(
            std::move(s->pa.strName),
            std::move(s->pa.strValue));
    }
}

const patt::Pattern
    ptCmdDefine     = (
        patt::Str("define") >> ptSpacing >>
        ptIdentifier / CaptureName >> ptSpacing >>
        ((-ptEndOfLine >> patt::Any()) % 1) / CaptureValue >>
        &ptEndOfLine) / HandleDefine;

static void
HandleUndef(io::IStream& is, const std::optional<patt::Match>& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* s = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = s->vecSources.back();

        std::string
            strMatch    = optMatch->GetString(is);
        if (strMatch.ends_with('\n'))
            strMatch.pop_back();
        cur_source.Message("found command: [#{}]", strMatch);
        
        auto
            itMacro     = s->mapGlobalMacros.find(s->pa.strName);
        if (itMacro == s->mapGlobalMacros.end())
            cur_source.Error("cannot remove non-existing macro {}", s->pa.strName);

        s->mapGlobalMacros.erase(itMacro);
    }
}

const patt::Pattern
    ptCmdUndef      = (
        patt::Str("undef") >> ptSpacing >>
        ptIdentifier / CaptureName >>
        &ptEndOfLine) / HandleUndef;

static void
HandleCommands(io::IStream&, const std::optional<patt::Match>& optMatch, const std::any& user_value)  {
    if (!optMatch) {
        auto&
            cur_source  = std::any_cast<PPState*>(user_value)->vecSources.back();
        cur_source.Error("unrecognized macro command");
    }
};

const patt::Pattern
    ptMacroCommand      = 
        patt::Str("#") >> (
            ptCmdDefine |=
            ptCmdUndef
        ) / HandleCommands;
const patt::Pattern
    ptOptMacroCommand   = ptMacroCommand % -1;

const patt::Pattern
    ptSourceText        =
        (-patt::Set("#$\"\'\n") >> -patt::Str("//") >> patt::Any()) % 1;
const patt::Pattern
    ptPreprocessor      = ptOptMacroCommand >> (
        ptSourceText |= (ptLineFeed >> ptOptMacroCommand) |= ptMacro |= ptStringLiteral |= ptCharLiteral |= ptLineComment
    ) % 0 >> patt::None();

inline int
TestPatterns() {
    io::cout.put("Testing Patterns:\n");
    
    PPState
    state           = {};
    std::string
        strFilename = "./assets/input.txt";
    auto
        sptrInput   = std::make_shared<io::IFileStream>(strFilename);

    state.vecSources.push_back({
        .strDescr       = strFilename,
        .sptrInput      = sptrInput,
        .setNewLines    = {}
    });
    try {
        auto
            optMatch    = ptPreprocessor->Eval(*sptrInput, &state);
        if (!optMatch) {
            auto&
                cur_source  = state.vecSources.back();
            cur_source.Error("failed to parse input text");
        }
        
        io::cout.put("Macro constants:\n");
        for (const auto& [strKey, strValue] : state.mapGlobalMacros) {
            io::cout.fmt(" > {}: {}\n", strKey, strValue);
        }
    }
    catch (const std::exception& err) {
        io::cerr.put(err.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}