#pragma once
#include <format>
#include <charconv>
#include <string_view>

#include "IOStreams.hpp"


namespace io {
    class SerialTextWriter {
    public:
        SerialTextWriter(SerialOStream& stream) :
            refStream(stream) {}

        SerialOStream&
        Stream() const noexcept {
            return this->refStream;
        }

        const auto&
        Char(this const auto& self, char c) {
            self.Stream().Write((std::byte)c);
            return self;
        }

        const auto&
        Endl(this const auto& self) {
            return self.Char('\n');
        }

        const auto&
        Str(this const auto& self, std::string_view strv) {
            for (char c : strv)
                self.Char(c);
            return self;
        }

        template<std::integral I>
        const auto&
        Int(this const auto& self, I val, int base = 10) {
            char
                lpcTemp[32] = {0};

            std::to_chars(
                std::begin(lpcTemp),
                std::end(lpcTemp),
                val, base);
            return self.Str(lpcTemp);
        }

        template<std::integral I>
        const auto&
        Dec(this const auto& self, I val) {
            return self.Int(val, 10);
        }

        template<std::integral I>
        const auto&
        Hex(this auto& self, I val) {
            return self.Int(val, 16);
        }

        template<std::integral I>
        const auto&
        Oct(this auto& self, I val) {
            return self.Int(val, 8);
        }

        template<std::integral I>
        const auto&
        Bin(this auto& self, I val) {
            return self.Int(val, 2);
        }

        template<std::floating_point F>
        const auto&
        Float(this auto& self, F val) {
            char
                lpcTemp[32] = {0};
            
            std::errc
                err = std::to_chars(
                        std::begin(lpcTemp),
                        std::end(lpcTemp),
                        val,
                        std::chars_format::fixed).ec;
            return self.Str((err != std::errc{})
                ? "NaN" : lpcTemp);
        }

        template<std::floating_point F>
        const auto&
        Float(this const auto& self, F val, int precision) {
            char
                lpcTemp[32] = {0};
            
            std::errc
                err = std::to_chars(
                        std::begin(lpcTemp),
                        std::end(lpcTemp),
                        val,
                        std::chars_format::fixed,
                        precision).ec;
            return self.Str((err != std::errc{})
                ? "NaN" : lpcTemp);
        }

        template<typename... Args>
        const auto&
        Fmt(this const auto& self, const std::format_string<Args...>& strfmt, Args&&... args) {
            return self.Str(
                std::format(
                    strfmt, std::forward<Args>(args)...));
        }

        template<std::floating_point F>
        const auto&
        Scientific(this const auto& self, F val) {
            char
                lpcTemp[32] = {0};
            
            std::errc
                err = std::to_chars(
                        std::begin(lpcTemp),
                        std::end(lpcTemp),
                        val,
                        std::chars_format::scientific).ec;
            return self.Str((err != std::errc{})
                ? "NaN" : lpcTemp);
        }

        template<std::floating_point F>
        const auto&
        Scientific(this const auto& self, F val, int precision) {
            char
                lpcTemp[32] = {0};
            
            std::errc
                err = std::to_chars(
                        std::begin(lpcTemp),
                        std::end(lpcTemp),
                        val,
                        std::chars_format::scientific,
                        precision).ec;
            return self.Str((err != std::errc{})
                ? "NaN" : lpcTemp);
        }

        template<typename V> requires
            std::same_as<char, V> ||
            std::constructible_from<std::string_view, V> ||
            std::integral<V> ||
            std::floating_point<V>
        const auto&
        Any(this const auto& self, const V& val) {
            if constexpr (std::same_as<V, char>)
                return self.Char(val);
            else if (std::is_constructible_v<std::string_view, V>)
                return self.Str({val});
            else if (std::is_integral_v<V>)
                return self.Int(val);
            else if constexpr (std::is_floating_point_v<V>)
                return self.Float(val);
        }

        const auto&
        Forward(this const auto& self, io::SerialIStream& from, size_t uCount = SIZE_MAX) {
            for (size_t i = 0; i != uCount; ++i) {
                std::optional<std::byte>
                    optc    = from.Read();
                if (!optc)
                    break;

                self.Stream().Write(*optc);
            }

            return self;
        }
    
    private:
        SerialOStream&
            refStream;
    };

    class TextWriter :
        public SerialTextWriter {
    public:
        TextWriter(OStream& stream) :
            SerialTextWriter(stream) {}

        OStream&
        Stream() const noexcept {
            return (OStream&)this->SerialTextWriter::Stream();
        }

        const auto&
        Go(this const auto& self, intptr_t offset, StreamOffsetOrigin origin = StreamOffsetOrigin::CurrentPos) {
            self.Stream().SetPosition(offset, origin);
            return self;
        }

        const auto&
        GoStart(this const auto& self) {
            return self.Go(0, StreamOffsetOrigin::StreamStart);
        }

        const auto&
        GoEnd(this const auto& self) {
            return self.Go(0, StreamOffsetOrigin::StreamEnd);
        }
    };

    class SerialTextReader {
    public:
        SerialTextReader(SerialIStream& stream) :
            refStream(stream) {}

        SerialIStream&
        Stream() const noexcept {
            return this->refStream;
        }

        const auto&
        Char(this const auto& self, char& out) {
            std::optional<std::byte>
                optc = self.Stream().Read();
            if (optc)
                out = (char)*optc;
            return self;
        }

        const auto&
        Line(this const auto& self, std::string& out) {
            std::string
                strLine;
            std::optional<std::byte>
                optc;
            while ((bool)(optc = self.Stream().Read()) && (char)*optc != '\n') {
                strLine += (char)*optc;
            }

            out = std::move(strLine);
            return self;
        }

        const auto&
        All(this const auto& self, std::string& out) {
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
        Forward(this const auto& self, io::SerialOStream& to, size_t uCount = SIZE_MAX) {
            for (size_t i = 0; i != uCount; ++i) {
                std::optional<std::byte>
                    optc    = self.Stream().Read();
                if (!optc)
                    break;

                to.Write(*optc);
            }

            return self;
        }

    private:
        SerialIStream&
            refStream;
    };

    class TextReader :
        public SerialTextReader {
    public:
        TextReader(IStream& stream) :
            SerialTextReader(stream) {}
        
        IStream&
        Stream() const noexcept {
            return (IStream&)this->SerialTextReader::Stream();
        }

        const auto&
        Go(this const auto& self, intptr_t offset, StreamOffsetOrigin origin = StreamOffsetOrigin::CurrentPos) {
            self.Stream().SetPosition(offset, origin);
            return self;
        }

        const auto&
        GoStart(this const auto& self) {
            return self.Go(0, StreamOffsetOrigin::StreamStart);
        }

        const auto&
        GoEnd(this const auto& self) {
            return self.Go(0, StreamOffsetOrigin::StreamEnd);
        }
    };
}