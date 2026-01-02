#include <classy-streams/IOStreams.hpp>
#include <classy-streams/ConsoleStreams.hpp>
#include <classy-streams/BufferStreams.hpp>
#include <classy-streams/FileStreams.hpp>
#include <classy-streams/IOReadWrite.hpp>
#include <classy-patterns/Patterns.hpp>
#include <stdexcept>
#include <memory>
#include <string>
#include <format>
#include <any>

struct SourceLocation {
    std::string
        strFrom;
    intptr_t
        iLine, iColumn;

    template<typename... Args>
    [[noreturn]] void
    Error(const std::format_string<Args...>& strfmt, Args&&... args) {
        std::string
            strMessage  = std::format("[{}: Ln {}, Col {}]\n > Error: {}\a",
                            this->strFrom,
                            this->iLine,
                            this->iColumn,
                            std::format(strfmt, std::forward<Args>(args)...));
        throw std::runtime_error(strMessage);
    }

    template<typename... Args>
    void
    Warning(const std::format_string<Args...>& strfmt, Args&&... args) {
        std::string
            strMessage  = std::format("[{}: Ln {}, Col {}]\n > Warning: {}\a",
                            this->strFrom,
                            this->iLine,
                            this->iColumn,
                            std::format(strfmt, std::forward<Args>(args)...));
        io::cerr.put(strMessage).put_endl();
    }

    template<typename... Args>
    void
    Message(const std::format_string<Args...>& strfmt, Args&&... args) {
        std::string
            strMessage  = std::format("[{}: Ln {}, Col {}]\n > {}",
                            this->strFrom,
                            this->iLine,
                            this->iColumn,
                            std::format(strfmt, std::forward<Args>(args)...));
        io::cout.put(strMessage).put_endl();
    }
};

struct InputSource {
    std::string
        strDescr;
    std::unique_ptr<io::IStream>
        uptrInput;
    std::flat_set<intptr_t>
        setNewLines;

    SourceLocation
    Where() const {
        intptr_t
            iCurPos     = this->uptrInput->GetPosition(),
            iLine       = (intptr_t)this->setNewLines.size() + 1,
            iLastLF     = (!this->setNewLines.empty())
                            ? *(this->setNewLines.cend() - 1)
                            : 0;
        return {
            .strFrom    = this->strDescr,
            .iLine      = iLine,
            .iColumn    = iCurPos - iLastLF + 1
        };
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
CountNewLine(io::IStream&, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* state     = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = state->vecSources.back();
        cur_source.setNewLines.emplace(
            cur_source.uptrInput->GetPosition());
    }
};

const patt::Pattern
    ptLineFeed  = (patt::Str("\n")),
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
        patt::Str("//") >> (patt::Any() - ptEndOfLine) % 0 >> &ptEndOfLine;

static void
CaptureName(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    auto* s = std::any_cast<PPState*>(user_data);
    if (optMatch) {
        s->pa.strName   = optMatch->GetString(is);
    }
    else {
        auto&
            cur_source  = s->vecSources.back();
        cur_source.Where().Error("expected an identifier");
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
        cur_source.Where().Error("failed to parse macro value");
    }
}

const patt::Pattern
    ptMacro         = (patt::Str("$") >> ptIdentifier / CaptureName);

static void
ExpandMacro(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data);

static void
HandleMacroUnexpected(io::IStream&, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto*
            state       = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = state->vecSources.back();

        cur_source.Where().Error("unexpected new line");
    }
}

const patt::Pattern
    ptMacroUnexpected   = (patt::Set("#\n") |= patt::Str("//")) / HandleMacroUnexpected;

const patt::Pattern
    ptExpandMacros  =
        (-ptMacroUnexpected >> (
            ((patt::Any() - patt::Set("$\"\'\n")) % 1) |=
            (ptMacro / ExpandMacro) |=
            ptStringLiteral |= ptCharLiteral)
        ) % 0 >> patt::None();

static void
ExpandMacro(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto&
            buffOuter   = dynamic_cast<io::IOBufferStream&>(is);
        auto*
            state       = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = state->vecSources.back();
    
        const std::string&
            strMacro    = state->pa.strName;
        auto
            itMacro     = state->mapGlobalMacros.find(strMacro);
        if (itMacro != state->mapGlobalMacros.end()) {
            cur_source.Where().Message("found macro ${}, expanding", strMacro);
            
            io::IOBufferStream
                buffInner;
            io::TextIO(buffInner)
                .put(itMacro->second)
                .go_start();
            
            ptExpandMacros->Eval(buffInner, user_data);
            
            buffInner.SetPosition(0);
            buffOuter.SetPosition(
                buffOuter.Replace(
                    optMatch->Begin(),
                    optMatch->End(),
                    buffInner));
        }
        else
            cur_source.Where().Error("macro ${} doesnt exist", strMacro);
    }
}

static void
HandleMacro(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_data) {
    if (optMatch) {
        auto* state     = std::any_cast<PPState*>(user_data);
        auto&
            cur_source  = state->vecSources.back();

        std::string
            strMacro    = state->pa.strName;
        cur_source.Where().Message("start expanding macro ${}", strMacro);
        
        io::IOBufferStream
           buff;
        optMatch->Forward(is, buff);
        buff.SetPosition(0);

        if (ptExpandMacros->Eval(buff, user_data)) {
            std::string
                strExpandedMacro;
            io::TextIO(buff)
                .go_start()
                .get_all(strExpandedMacro);
            
            cur_source.Where().Message("expanded macro ${} to [{}]", strMacro, strExpandedMacro);
        }
        else
            cur_source.Where().Error("failed to handle macro ${}", strMacro);
    }
}

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
        cur_source.Where().Message("found command: [#{}]", strMatch);

        if (s->mapGlobalMacros.contains(s->pa.strName))
            cur_source.Where().Error("${} macro is already defined", s->pa.strName);

        s->mapGlobalMacros.emplace(
            std::move(s->pa.strName),
            std::move(s->pa.strValue));
    }
}

const patt::Pattern
    ptCmdDefine     = (
        patt::Str("define") >> ptSpacing >>
        ptIdentifier / CaptureName >> ptSpacing >>
        ((patt::Any() - ptEndOfLine) % 1) / CaptureValue >>
        ptOptSpacing >> &ptEndOfLine) / HandleDefine;

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
        cur_source.Where().Message("found command: [#{}]", strMatch);
        
        auto
            itMacro     = s->mapGlobalMacros.find(s->pa.strName);
        if (itMacro != s->mapGlobalMacros.end())
            s->mapGlobalMacros.erase(itMacro);
        else
            cur_source.Where().Warning("cannot remove non-existing macro {}", s->pa.strName);
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
        cur_source.Where().Error("unrecognized macro command");
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
        (patt::Any() - patt::Set("#$\"\'\n") - patt::Str("//") >> patt::Any()) % 1;
const patt::Pattern
    ptPreprocessor      = ptOptMacroCommand >> (
        ptSourceText |= (ptLineFeed / CountNewLine >> ptOptMacroCommand) |= ptMacro / HandleMacro |= ptStringLiteral |= ptCharLiteral |= ptLineComment
    ) % 0 >> patt::None();

extern int
main() {
    io::cout.put("CTEST_FULL_OUTPUT\n");
    io::cout.put("Proto preprocessor:\n");
    
    PPState
        state           = {};
    std::string
        strFilename = "./assets/input.txt";

    state.vecSources.emplace_back(
        strFilename,
        std::make_unique<io::IFileStream>(strFilename));
    try {
        auto
            optMatch    = ptPreprocessor->Eval(*state.vecSources.back().uptrInput, &state);
        if (!optMatch) {
            auto&
                cur_source  = state.vecSources.back();
            cur_source.Where().Error("failed to parse input text");
        }
        
        io::cout.put("Macro constants:\n");
        for (const auto& [strKey, strValue] : state.mapGlobalMacros) {
            io::cout.fmt(" > {}: [{}]\n", strKey, strValue);
        }
    }
    catch (const std::exception& err) {
        io::cerr.put(err.what()).put_endl();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}