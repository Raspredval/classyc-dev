#pragma once
#include <FileViewStreams.hpp>
#include <TextIO.hpp>

namespace io {
    static IFileViewStream
        std_input(stdin);
    static OFileViewStream
        std_output(stdout),
        std_error(stderr);

    static TextReader
        cin(std_input);
    static TextWriter
        cout(std_output),
        cerr(std_error);
}
