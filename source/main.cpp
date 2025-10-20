#include <my-iostreams/ConsoleStreams.hpp>
#include <my-iostreams/BufferStreams.hpp>
#include <my-patterns/Patterns.hpp>

int
main() {
    patt::Grammar
        gr;

    gr["filename"]  =
        (patt::Str("..") |= patt::Str(".") |= (patt::Alnum() % 1)) >>
        (patt::Str("/") >> gr["filename"]) % -1;
    patt::Pattern
        ptFilepath  =
            (patt::Str("/") |= patt::Str("~/")) % -1 >>
            gr["filename"] % -1 >>
            patt::Endl();

    io::IOBufferStream
        buffTest;

    std::string
        strInput;
    io::cin.Line(strInput);
    io::cout.Fmt("input: \"{}\"\n", strInput);

    io::TextWriter(buffTest)
        .Str(strInput)
        .Str("\r\n")
        .GoStart();

    auto
        optMatch    = patt::SafeEval(ptFilepath, buffTest);
    if (!optMatch) {
        io::cerr.Str(optMatch.error().what()).Endl();
        return -1;
    }

    io::cout.Fmt("matched: \"{}\"\n", optMatch->GetString(buffTest));
    return 0;
}