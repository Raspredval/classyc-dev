#pragma once
#include <my-iostreams/IOStreams.hpp>

namespace Preprocessor {
    class PreprocessorState {
    public:
        void
        Preprocess(io::IStream& input, io::OStream& output);

    private:
        
    };
}
