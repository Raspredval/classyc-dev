#pragma once
#include "CFile.hpp"

namespace io {
    namespace __impl {
        class FileStreamBase :
            virtual public  StreamState,
            virtual public  StreamPosition,
            protected       CFile {
        public:
            FileStreamBase() = default;
            FileStreamBase(std::string_view strvFilename, std::string_view strvMode) :
                CFile(strvFilename, strvMode) {}

            bool
            EndOfStream() const noexcept override {
                return this->CFile::EndOfStream();
            }

            bool
            Good() const noexcept override {
                return !this->CFile::Bad();
            }

            intptr_t
            GetPosition() const noexcept override {
                return this->CFile::GetPosition();
            }

            bool
            SetPosition(intptr_t offset, StreamOffsetOrigin from = StreamOffsetOrigin::StreamStart) override {
                return this->CFile::SetPosition(offset, from);
            }

            bool
            Flush() {
                return this->CFile::Flush();
            }

            FILE*
            FileHandle() const noexcept {
                return this->CFile::Handle();
            }
        };

        class IFileStream :
            public              IStream,
            virtual public      FileStreamBase {
        protected:
            IFileStream() = default;

        public:
            IFileStream(std::string_view strvFilename) :
                FileStreamBase(strvFilename, "r") {}

            std::optional<std::byte>
            Read() override {
                return this->CFile::Read();
            }

            size_t
            ReadSome(std::span<std::byte> buffer) override {
                return this->CFile::ReadSome(buffer);
            }
        };
    }

    class OFileStream :
        public              OStream,
        virtual public      __impl::FileStreamBase {
    protected:
        OFileStream() = default;

    public:
        OFileStream(std::string_view strvFilename) :
            FileStreamBase(strvFilename, "w") {}

        bool
        Write(std::byte c) override {
            return this->CFile::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->CFile::WriteSome(buffer);
        }
    };

    class IOFileStream :
        public          IOStream,
        virtual public  __impl::FileStreamBase {
    public:
        IOFileStream(std::string_view strvFilename) :
            FileStreamBase(strvFilename, "r+") {}

        bool
        Write(std::byte c) override {
            return this->CFile::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->CFile::WriteSome(buffer);
        }

        std::optional<std::byte>
        Read() override {
            return this->CFile::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->CFile::ReadSome(buffer);
        }
    };
}

