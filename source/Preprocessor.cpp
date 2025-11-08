#include <my-iostreams/IOStreams.hpp>
#include <my-patterns/Patterns.hpp>

#include <Preprocessor.hpp>


const patt::Pattern
    ptSpacing       = patt::Space() % 1,
    ptOptSpacing    = patt::Space() % 0;
const patt::Pattern
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
        patt::Str("\'");
        
extern bool
Preprocess(io::IStream& input, io::OStream& output, std::string_view strvInputDescr) {
    
}