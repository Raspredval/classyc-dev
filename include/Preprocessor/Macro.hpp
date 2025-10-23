#pragma once
#include <memory>
#include <my-iostreams/BufferStreams.hpp>

namespace Preprocessor {
    namespace __impl {
        class Macro;
    }
    
    using Macro =
        std::shared_ptr<__impl::Macro>;

    namespace __impl {
        class Macro {
        public:
            virtual void
            Expand(io::IOBufferStream& buffLine, intptr_t iReplaceBegin, intptr_t iReplaceEnd) = 0;

            virtual ~Macro() = default;

            static void
            ExpandMacros(io::IOBufferStream& buffLine);
        };
    }
}