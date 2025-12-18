#pragma once
#include <string_view>
#include <classy-streams/IOStreams.hpp>

extern bool
Preprocess(io::IStream& input, io::OStream& output, std::string_view strvInputDescr = "input_source");
