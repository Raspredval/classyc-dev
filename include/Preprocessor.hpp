#pragma once
#include <string_view>
#include <my-iostreams/IOStreams.hpp>

extern bool
Preprocess(io::IStream& input, io::OStream& output, std::string_view strvInputDescr = "input_source");
