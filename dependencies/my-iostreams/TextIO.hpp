#pragma once
#include <format>
#include <charconv>
#include <string_view>

#include "IOStreams.hpp"


namespace io {
    namespace __impl {
        class SerialTextOutput {
        public:
            const auto&
            put_char(this const auto& self, char c) {
                self.stream().Write((std::byte)c);
                return self;
            }

            const auto&
            put_endl(this const auto& self) {
                return self.put_char('\n');
            }

            const auto&
            put_str(this const auto& self, std::string_view strv) {
                for (char c : strv)
                    self.put_char(c);
                return self;
            }

            template<std::integral I>
            const auto&
            put_int(this const auto& self, I val, int base = 10) {
                char
                    lpcTemp[32] = {0};

                std::to_chars(
                    std::begin(lpcTemp),
                    std::end(lpcTemp),
                    val, base);
                return self.put_str(lpcTemp);
            }

            template<std::integral I>
            const auto&
            put_dec(this const auto& self, I val) {
                return self.put_int(val, 10);
            }

            template<std::integral I>
            const auto&
            put_hex(this auto& self, I val) {
                return self.put_int(val, 16);
            }

            template<std::integral I>
            const auto&
            put_oct(this auto& self, I val) {
                return self.put_int(val, 8);
            }

            template<std::integral I>
            const auto&
            put_bin(this auto& self, I val) {
                return self.put_int(val, 2);
            }

            template<std::floating_point F>
            const auto&
            put_float(this auto& self, F val) {
                char
                    lpcTemp[32] = {0};
                
                std::errc
                    err = std::to_chars(
                            std::begin(lpcTemp), std::end(lpcTemp),
                            val,
                            std::chars_format::fixed).ec;
                return self.put_str((err != std::errc{})
                    ? "NaN" : lpcTemp);
            }

            template<std::floating_point F>
            const auto&
            put_float(this const auto& self, F val, int precision) {
                char
                    lpcTemp[32] = {0};
                
                std::errc
                    err = std::to_chars(
                            std::begin(lpcTemp), std::end(lpcTemp),
                            val,
                            std::chars_format::fixed,
                            precision).ec;
                return self.put_str((err != std::errc{})
                    ? "NaN" : lpcTemp);
            }

            template<typename... Args>
            const auto&
            fmt(this const auto& self, const std::format_string<Args...>& strfmt, Args&&... args) {
                static char
                    lpcBuffer[256];
                auto
                    result  = std::format_to_n(
                                lpcBuffer, sizeof(lpcBuffer),
                                strfmt, std::forward<Args>(args)...);

                return self.put_str(std::string_view(lpcBuffer, (size_t)result.size));
            }

            template<typename V> requires
                std::same_as<char, V> ||
                std::constructible_from<std::string_view, V> ||
                std::integral<V> ||
                std::floating_point<V>
            const auto&
            put(this const auto& self, const V& val) {
                if constexpr (std::same_as<V, char>)
                    return self.put_char(val);
                else if constexpr (std::constructible_from<std::string_view, V>)
                    return self.put_str({val});
                else if constexpr (std::integral<V>)
                    return self.put_int(val);
                else if constexpr (std::floating_point<V>)
                    return self.put_float(val);
                else
                    return self;
            }

            const auto&
            forward_from(this const auto& self, io::SerialIStream& from, size_t uCount = SIZE_MAX) {
                for (size_t i = 0; i != uCount; ++i) {
                    std::optional<std::byte>
                        optc    = from.Read();
                    if (!optc)
                        break;

                    self.stream().Write(*optc);
                }

                return self;
            }

            const auto&
            forward_from(this const auto& self, std::string& from, size_t uCount = SIZE_MAX) {
                for (size_t i = 0; i != std::min(uCount, from.size()); ++i) {
                    self.stream().Write((std::byte)from[i]);
                }

                return self;
            }
        };

        class SerialTextInput {
        public:
            const auto&
            get_char(this const auto& self, char& out) {
                std::optional<std::byte>
                    optc = self.stream().Read();
                if (optc)
                    out = (char)*optc;
                return self;
            }

            const auto&
            get_word(this const auto& self, std::string& out) {
                std::optional<std::byte>
                    optc;
                while ((bool)(optc = self.stream().Read())) {
                    if (!isspace((int)*optc)) {
                        self.stream().PutBack(*optc);
                        break;
                    }
                }
                while ((bool)(optc = self.stream().Read())) {
                    if (!isspace((int)*optc)) {
                        out.push_back((char)*optc);
                    }
                    else {
                        self.stream().PutBack(*optc);
                        break;
                    }
                }

                return self;
            }

            const auto&
            get_all(this const auto& self, std::string& out) {
                std::string
                    strAll;
                std::optional<std::byte>
                    optc;
                while ((bool)(optc = self.stream().Read())) {
                    strAll += (char)*optc;
                }

                out = std::move(strAll);
                return self;
            }

            const auto&
            get_line(this const auto& self, std::string& out) {
                std::string
                    strLine;
                std::optional<std::byte>
                    optc;
                while ((bool)(optc = self.stream().Read()) && (char)*optc != '\n') {
                    strLine += (char)*optc;
                }

                out = std::move(strLine);
                return self;
            }

            const auto&
            get_dec(this const auto& self, std::integral auto& out) {
                return self.get_int_impl(
                    out, 10,
                    [](char c) -> bool {
                        return c >= '0' && c <= '9';
                    });
            }

            const auto&
            get_hex(this const auto& self, std::integral auto& out) {
                return self.get_int_impl(
                    out, 16,
                    [](char c) -> bool {
                        return
                            (c >= '0' && c <= '9') ||
                            (c >= 'a' && c <= 'f') ||
                            (c >= 'A' && c <= 'F');
                    });
            }

            const auto&
            get_oct(this const auto& self, std::integral auto& out) {
                return self.get_int_impl(
                    out, 8,
                    [](char c) -> bool {
                        return c >= '0' && c <= '7';
                    });
            }

            const auto&
            get_bin(this const auto& self, std::integral auto& out) {
                return self.get_int_impl(
                    out, 2,
                    [](char c) -> bool {
                        return c >= '0' && c <= '1';
                    });
            }

            const auto&
            get_int(this const auto& self, std::integral auto& out, int base = 10) {
                switch (base) {
                case 2:
                    return self.get_bin(out);
                
                case 8:
                    return self.get_oct(out);

                case 10:
                    return self.get_dec(out);

                case 16:
                    return self.get_hex(out);

                default:
                    return self;
                }
            }

            const auto&
            get_float(this const auto& self, std::floating_point auto& out) {
                char
                    lpcBuffer[32];
                size_t
                    uSize = 0;
                std::optional<std::byte>
                    optc;
            
            ParseSpacing:
                if ((bool)(optc = self.stream().Read())) {
                    if (!isspace((int)*optc)) {
                        self.stream().PutBack(*optc);
                        goto ParseFirstChar;
                    }
                    else
                        goto ParseSpacing;
                }
                else
                    return self;

            ParseFirstChar:
                if ((bool)(optc = self.stream().Read())) {
                    char c = (char)*optc;
                    
                    if (c == '-' || c == '+' || isdigit(c)) {
                        lpcBuffer[uSize] = c;
                        uSize += 1;
                        goto ParseNaturalPart;
                    }
                    else if (c == '.' || c == ',') {
                        lpcBuffer[uSize] = '.';
                        uSize += 1;
                        goto ParseFractionalPart;
                    }
                    else {
                        self.stream().PutBack(*optc);
                        return self;
                    }
                }
                else
                    return self;

            ParseNaturalPart:
                if (uSize == sizeof(lpcBuffer))
                    goto GenerateValue;
                if ((bool)(optc = self.stream().Read())) {
                    char c = (char)*optc;
                    if (isdigit(c)) {
                        lpcBuffer[uSize] = c;
                        uSize += 1;
                        goto ParseNaturalPart;
                    }
                    else if (c == '.' || c == ',') {
                        lpcBuffer[uSize] = '.';
                        uSize += 1;
                        goto ParseFractionalPart;
                    }
                    else {
                        self.stream().PutBack(*optc);
                        goto GenerateValue;
                    }
                }
                else
                    goto GenerateValue;

            ParseFractionalPart:
                if (uSize == sizeof(lpcBuffer))
                    goto GenerateValue;
                if ((bool)(optc = self.stream().Read())) {
                    char c = (char)*optc;
                    if (isdigit(c)) {
                        lpcBuffer[uSize] = c;
                        uSize += 1;
                        goto ParseFractionalPart;
                    }
                    else {
                        self.stream().PutBack(*optc);
                        goto GenerateValue;
                    }
                }
                else
                    goto GenerateValue;

            GenerateValue:
                std::from_chars(
                    lpcBuffer, lpcBuffer + uSize,
                    out, std::chars_format::fixed);
                return self;
            }

            template<typename V> requires
                std::same_as<char, V> ||
                std::same_as<std::string, V> ||
                std::integral<V> ||
                std::floating_point<V>
            const auto&
            get(this const auto& self, V& val) {
                if constexpr (std::same_as<char, V>)
                    return self.get_char(val);
                else if constexpr (std::constructible_from<std::string_view, V>)
                    return self.get_word(val);
                else if constexpr (std::integral<V>)
                    return self.get_int(val);
                else if constexpr (std::floating_point<V>)
                    return self.get_float(val);
                else
                    return self;
            }

            const auto&
            forward_to(this const auto& self, io::SerialOStream& to, size_t uCount = SIZE_MAX) {
                for (size_t i = 0; i != uCount; ++i) {
                    std::optional<std::byte>
                        optc    = self.stream().Read();
                    if (!optc)
                        break;

                    to.Write(*optc);
                }

                return self;
            }

            const auto&
            forward_to(this const auto& self, std::string& to, size_t uCount = SIZE_MAX) {
                for (size_t i = 0; i != uCount; ++i) {
                    std::optional<std::byte>
                        optc    = self.stream().Read();
                    if (!optc)
                        break;

                    to.push_back((char)*optc);
                }

                return self;
            }

        protected:
            const auto&
            get_int_impl(this const auto& self, std::integral auto& out, int base, bool(*fnIsDigit)(char)) {
                char
                    lpcBuffer[32];
                size_t
                    uSize = 0;
                std::optional<std::byte>
                    optc;
                
            ParseSpacing:
                if ((bool)(optc = self.stream().Read())) {
                    if (!isspace((int)*optc)) {
                        self.stream().PutBack(*optc);
                        goto ParseFirstChar;
                    }
                    else
                        goto ParseSpacing;
                }
                else
                    return self;

            ParseFirstChar:
                if ((bool)(optc = self.stream().Read())) {
                    char c = (char)*optc;
                    
                    if (c == '-' || c == '+' || isdigit(c)) {
                        lpcBuffer[uSize] = c;
                        uSize += 1;
                        goto ParseDigits;
                    }
                    else {
                        self.stream().PutBack(*optc);
                        return self;
                    }
                }
                else
                    return self;

            ParseDigits:
                if (uSize == sizeof(lpcBuffer))
                    goto GenerateValue;
                if ((bool)(optc = self.stream().Read())) {
                    char c = (char)*optc;
                    if (fnIsDigit(c)) {
                        lpcBuffer[uSize] = c;
                        uSize += 1;
                        goto ParseDigits;
                    }
                    else {
                        self.stream().PutBack(*optc);
                        goto GenerateValue;
                    }
                }
                else
                    goto GenerateValue;

            GenerateValue:
                std::from_chars(
                    lpcBuffer, lpcBuffer + uSize,
                    out, base);
                return self;
            }
        };
        
        class RandomAccessTextIO {
        public:
            const auto&
            go(this const auto& self, intptr_t offset, StreamOffsetOrigin origin = StreamOffsetOrigin::CurrentPos) {
                self.stream().SetPosition(offset, origin);
                return self;
            }

            const auto&
            go_start(this const auto& self) {
                return self.go(0, StreamOffsetOrigin::StreamStart);
            }

            const auto&
            go_end(this const auto& self) {
                return self.go(0, StreamOffsetOrigin::StreamEnd);
            }
        };
    }

    class SerialTextInput :
        public __impl::SerialTextInput {
    public:
        SerialTextInput(const SerialTextInput&) = delete;
        
        SerialTextInput(io::SerialIStream& is) :
            refStream(is) {}

        auto&
        stream() const noexcept {
            return this->refStream;
        }

    private:
        io::SerialIStream&
            refStream;
    };

    class SerialTextOutput :
        public __impl::SerialTextOutput {
    public:
        SerialTextOutput(const SerialTextOutput&) = delete;
        
        SerialTextOutput(io::SerialOStream& os) :
            refStream(os) {}

        auto&
        stream() const noexcept {
            return this->refStream;
        }

    private:
        io::SerialOStream&
            refStream;
    };

    class SerialTextIO :
        public __impl::SerialTextInput,
        public __impl::SerialTextOutput {
    public:
        SerialTextIO(const SerialTextIO&) = delete;

        SerialTextIO(io::SerialIOStream& ios) :
            refStream(ios) {}

        auto&
        stream() const noexcept {
            return this->refStream;
        }
    
    private:
        io::SerialIOStream&
            refStream;
    };

    class TextInput :
        public __impl::RandomAccessTextIO,
        public __impl::SerialTextInput {
    public:
        TextInput(const TextInput&) = delete;
        
        TextInput(io::IStream& is) :
            refStream(is) {}

        auto&
        stream() const noexcept {
            return this->refStream;
        }

    private:
        io::IStream&
            refStream;
    };

    class TextOutput :
        public __impl::RandomAccessTextIO,
        public __impl::SerialTextOutput {
    public:
        TextOutput(const TextOutput&) = delete;
        
        TextOutput(io::OStream& os) :
            refStream(os) {}

        auto&
        stream() const noexcept {
            return this->refStream;
        }

    private:
        io::OStream&
            refStream;
    };

    class TextIO :
        public __impl::RandomAccessTextIO,
        public __impl::SerialTextInput,
        public __impl::SerialTextOutput {
    public:
        TextIO(const TextIO&) = delete;

        TextIO(io::IOStream& ios) :
            refStream(ios) {}

        auto&
        stream() const noexcept {
            return this->refStream;
        }
    
    private:
        io::IOStream&
            refStream;
    };
}