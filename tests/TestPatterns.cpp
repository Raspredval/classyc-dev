#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-patterns/Patterns.hpp>

const patt::Pattern
    ptIdentifier    =
        (patt::Alpha() |= patt::Str("_")) >>
        (patt::Alnum() |= patt::Str("_")) % 0;

static void
HandleCommand(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_value) {
    if (optMatch) {
        patt::OptMatch
            optIDMatch  = ptIdentifier->Eval(is, user_value);
        if (optIDMatch) {
            std::string
                strIdentifier   = optIDMatch->GetString(is);
            io::cout.fmt("Parsed ID: {}\n", strIdentifier);
        }
    }
}

const patt::Pattern
    ptCommand       =
        patt::Str("#") / HandleCommand >> patt::None();

extern int
main() {
    io::cout.put("CTEST_FULL_OUTPUT\n");
    io::cout.put("Testing patterns\n");

    io::IOBufferStream
        buffTest;
    
    io::TextIO(buffTest)
        .put("#fancy_name")
        .go_start();

    patt::OptMatch
        optmCommand = ptCommand->Eval(buffTest);
    if (!optmCommand) {
        io::cerr.put("Failed to match command\n");
        return EXIT_FAILURE;
    }

    io::cout.put("Parsing from the handler callback was successfull\n");
    return EXIT_SUCCESS;
}