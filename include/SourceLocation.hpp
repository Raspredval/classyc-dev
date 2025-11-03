#pragma once
#include <format>
#include <string>
#include <string_view>
#include <stdexcept>

struct SourceLocation {
    size_t
        uLine;
    std::string
        strFile;
};

inline void
SourceError(const SourceLocation& srcloc, std::string_view strvMsg) {
    throw std::runtime_error(
        std::format(
            "[{}; line {}] Error: {}\n",
            srcloc.strFile,
            srcloc.uLine,
            strvMsg)
    );
}

inline void
SourceAssert(bool bCond, const SourceLocation& srcloc, std::string_view strvMsg) {
    if (!bCond)
        SourceError(srcloc, strvMsg);
}