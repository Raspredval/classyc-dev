#include <classy-streams/ConsoleStreams.hpp>
#include <classy-streams/BufferStreams.hpp>
#include <classy-streams/Patterns.hpp>

const patt::Pattern
    ptIdentifier    =
        (patt::Alpha() |= patt::Str("_")) >>
        (patt::Alnum() |= patt::Str("_")) % 0;

static void
HandleCommand(io::IStream& is, const patt::OptMatch& optMatch, const std::any& user_value) {
    if (optMatch) {
        patt::OptMatch
            optIDMatch  = ptIdentifier->Eval(is, user_value);
        if (!optIDMatch)
            throw std::runtime_error("failed to parse identifier");
        
        std::string
            strIdentifier   = optIDMatch->GetString(is);
        io::cout.fmt("Parsed ID: {}\n", strIdentifier);
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

    try {
        patt::OptMatch
            optCommandMatch = ptCommand->Eval(buffTest);
        if (!optCommandMatch)
            throw std::runtime_error("failed to match command");

        io::cout.put("parsing from the handler callback was successfull\n");
        return EXIT_SUCCESS;
    }
    catch(const std::exception& exc) {
        io::cerr.put(exc.what()).put_endl();
        return EXIT_FAILURE;
    }
}