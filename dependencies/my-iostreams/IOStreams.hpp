#pragma once
#include <span>
#include <cstdint>
#include <optional>

namespace io {
    enum class StreamOffsetOrigin {
        StreamStart = 0,    // SEEK_SET
        CurrentPos  = 1,    // SEEK_CUR
        StreamEnd   = 2     // SEEK_END
    };

    namespace __impl {
        class StreamState {
        public:
            StreamState() = default;
            virtual ~StreamState() = default;

            virtual bool
            EndOfStream() const noexcept = 0;

            virtual bool
            Good() const noexcept = 0;

            inline operator bool() const noexcept {
                return this->Good();
            }
        };

        class StreamPosition {
        public:
            StreamPosition() = default;
            virtual ~StreamPosition() = default;

            virtual intptr_t
            GetPosition() const noexcept = 0;

            virtual bool
            SetPosition(
                intptr_t            offset, 
                StreamOffsetOrigin  from = StreamOffsetOrigin::StreamStart) = 0;
        };
    }

    class SerialIStream :
        virtual public  __impl::StreamState {
    public:
        SerialIStream() = default;
        virtual ~SerialIStream() = default;

        virtual std::optional<std::byte>
        Read() = 0;
    
        virtual size_t
        ReadSome(
            std::span<std::byte> buffer) = 0;

        virtual bool
        PutBack(std::byte c) = 0;
    };

    class SerialOStream :
        virtual public  __impl::StreamState {
    public:
        SerialOStream() = default;
        virtual ~SerialOStream() = default;

        virtual bool
        Write(std::byte c) = 0;

        virtual size_t
        WriteSome(
            std::span<const std::byte> buffer) = 0;
    };

    class SerialIOStream :
        public  SerialIStream,
        public  SerialOStream {};

    class IStream :
        virtual public  __impl::StreamPosition,
        public          SerialIStream {};

    class OStream :
        virtual public  __impl::StreamPosition,
        public          SerialOStream {};

    class IOStream :
        public  IStream,
        public  OStream {};
}
