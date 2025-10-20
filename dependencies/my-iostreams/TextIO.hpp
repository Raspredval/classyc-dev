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
            putc(this const auto& self, char c) {
                self.stream().Write((std::byte)c);
                return self;
            }

            const auto&
            putnl(this const auto& self) {
                return self.putc('\n');
            }

            const auto&
            puts(this const auto& self, std::string_view strv) {
                for (char c : strv)
                    self.putc(c);
                return self;
            }

            template<std::integral I>
            const auto&
            puti(this const auto& self, I val, int base = 10) {
                char
                    lpcTemp[32] = {0};

                std::to_chars(
                    std::begin(lpcTemp),
                    std::end(lpcTemp),
                    val, base);
                return self.puts(lpcTemp);
            }

            template<std::integral I>
            const auto&
            putd(this const auto& self, I val) {
                return self.puti(val, 10);
            }

            template<std::integral I>
            const auto&
            putx(this auto& self, I val) {
                return self.puti(val, 16);
            }

            template<std::integral I>
            const auto&
            puto(this auto& self, I val) {
                return self.puti(val, 8);
            }

            template<std::integral I>
            const auto&
            putb(this auto& self, I val) {
                return self.puti(val, 2);
            }

            template<std::floating_point F>
            const auto&
            putf(this auto& self, F val) {
                char
                    lpcTemp[32] = {0};
                
                std::errc
                    err = std::to_chars(
                            std::begin(lpcTemp),
                            std::end(lpcTemp),
                            val,
                            std::chars_format::fixed).ec;
                return self.puts((err != std::errc{})
                    ? "NaN" : lpcTemp);
            }

            template<std::floating_point F>
            const auto&
            putf(this const auto& self, F val, int precision) {
                char
                    lpcTemp[32] = {0};
                
                std::errc
                    err = std::to_chars(
                            std::begin(lpcTemp),
                            std::end(lpcTemp),
                            val,
                            std::chars_format::fixed,
                            precision).ec;
                return self.puts((err != std::errc{})
                    ? "NaN" : lpcTemp);
            }

            template<typename... Args>
            const auto&
            fmt(this const auto& self, const std::format_string<Args...>& strfmt, Args&&... args) {
                return self.puts(
                    std::format(
                        strfmt, std::forward<Args>(args)...));
            }

            template<typename V> requires
                std::same_as<char, V> ||
                std::constructible_from<std::string_view, V> ||
                std::integral<V> ||
                std::floating_point<V>
            const auto&
            put(this const auto& self, const V& val) {
                if constexpr (std::same_as<V, char>)
                    return self.putc(val);
                else if (std::is_constructible_v<std::string_view, V>)
                    return self.puts({val});
                else if (std::is_integral_v<V>)
                    return self.puti(val);
                else if constexpr (std::is_floating_point_v<V>)
                    return self.putf(val);
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
        };

        class SerialTextInput {
        public:
            const auto&
            getc(this const auto& self, char& out) {
                std::optional<std::byte>
                    optc = self.stream().Read();
                if (optc)
                    out = (char)*optc;
                return self;
            }

            const auto&
            gets(this const auto& self, std::string& out) {
                std::string
                    strAll;
                std::optional<std::byte>
                    optc;
                while ((bool)(optc = self.Stream().Read())) {
                    strAll += (char)*optc;
                }

                out = std::move(strAll);
                return self;
            }

            const auto&
            getln(this const auto& self, std::string& out) {
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
            forward_to(this const auto& self, io::SerialOStream& to, size_t uCount = SIZE_MAX) {
                for (size_t i = 0; i != uCount; ++i) {
                    std::optional<std::byte>
                        optc    = self.Stream().Read();
                    if (!optc)
                        break;

                    to.Write(*optc);
                }

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