#pragma once
#include <cstdio>
#include <format>
#include <stdexcept>
#include <string_view>

#include "IOStreams.hpp"


namespace io {
    namespace __impl {
        class FileViewStreamBase :
            virtual public  StreamState,
            virtual public  StreamPosition {
        public:
            FileViewStreamBase() = default;

            FileViewStreamBase(FILE* handle) :
                handle(handle) {}

            [[nodiscard]] bool
            EndOfStream() const noexcept override {
                return feof(this->handle);
            }

            [[nodiscard]] bool
            Good() const noexcept override {
                return !ferror(this->handle);
            }

            void
            ClearFlags() noexcept override {
                return clearerr(this->handle);
            }

            [[nodiscard]] intptr_t
            GetPosition() const noexcept override {
                return ftell(this->handle);
            }

            bool
            SetPosition(
                intptr_t             offset,
                StreamOffsetOrigin  from = StreamOffsetOrigin::StreamStart) override
            {
                return !fseek(
                    this->handle,
                    offset,
                    (int)from);
            }

            [[nodiscard]] FILE*
            Handle() const noexcept {
                return this->handle;
            }

            bool
            Flush() {
                return !fflush(this->handle);
            }

        protected:
            std::optional<std::byte>
            Read() {
                auto c =
                    fgetc(this->handle);
                return (c != EOF)
                    ? std::optional{ (std::byte)c }
                    : std::nullopt;
            }

            size_t
            ReadSome(std::span<std::byte> buffer) {
                return fread(
                    buffer.data(),
                    1, buffer.size(),
                    this->handle);
            }

            bool
            Write(std::byte c) {
                auto result =
                    fputc((int)c, this->handle);
                return result != EOF;
            }

            size_t
            WriteSome(std::span<const std::byte> buffer) {
                return fwrite(
                    buffer.data(),
                    1, buffer.size(),
                    this->handle);
            }

            bool
            PutBack(std::byte c) {
                return ungetc((int)c, this->handle) != EOF;
            }

            FILE*
                handle = nullptr;
        };

        class FileStreamBase :
            public FileViewStreamBase {
        private:
            FileStreamBase(FILE* handle) noexcept :
                FileViewStreamBase(handle) {}

        public:
            FileStreamBase() = default;
            FileStreamBase(const FileStreamBase&) = delete;
            
            FileStreamBase(FileStreamBase&& obj) noexcept {
                this->handle    = obj.handle;
                obj.handle      = nullptr;
            }

            auto&
            operator=(FileStreamBase&& obj) noexcept {
                FileStreamBase
                    temp    = std::move(obj);
                std::swap(
                    this->handle, temp.handle);
                return *this;
            }

            FileStreamBase(std::string_view strvFilename, std::string_view strvMode) {
                this->handle    = fopen(
                                    strvFilename.data(),
                                    strvMode.data());
                if (this->handle == nullptr) {
                    throw std::runtime_error(std::format(
                        "failed to open file {} with mode {}",
                        strvFilename, strvMode));
                }
            }

            ~FileStreamBase() noexcept {
                if (this->handle != nullptr)
                    fclose(this->handle);
            }
        };
    }

    class IFileViewStream :
        public  IStream,
        public  __impl::FileViewStreamBase {
    protected:
        IFileViewStream() = default;

    public:
        IFileViewStream(FILE* handle) :
            FileViewStreamBase(handle) {}
        
        std::optional<std::byte>
        Read() override {
            return this->FileViewStreamBase::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->FileViewStreamBase::ReadSome(buffer);
        }

        bool
        PutBack(std::byte c) override {
            return this->FileViewStreamBase::PutBack(c);
        }
    };

    class OFileViewStream :
        public  OStream,
        public  __impl::FileViewStreamBase {
    protected:
        OFileViewStream() = default;

    public:
        OFileViewStream(FILE* handle) :
            FileViewStreamBase(handle) {}
        
        bool
        Write(std::byte c) override {
            return this->FileViewStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->FileViewStreamBase::WriteSome(buffer);
        }
    };

    class IOFileViewStream :
        public  IOStream,
        public  __impl::FileViewStreamBase {
    public:
        IOFileViewStream(FILE* handle) :
            FileViewStreamBase(handle) {}

        std::optional<std::byte>
        Read() override {
            return this->FileViewStreamBase::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->FileViewStreamBase::ReadSome(buffer);
        }

        bool
        PutBack(std::byte c) override {
            return this->FileViewStreamBase::PutBack(c);
        }

        bool
        Write(std::byte c) override {
            return this->FileViewStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->FileViewStreamBase::WriteSome(buffer);
        }
    };

    class IFileStream :
        public  IStream,
        public  __impl::FileStreamBase {
    protected:
        IFileStream() = default;

    public:
        IFileStream(std::string_view strvFilename) :
            FileStreamBase(strvFilename, "r") {}

        std::optional<std::byte>
        Read() override {
            return this->FileStreamBase::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->FileStreamBase::ReadSome(buffer);
        }

        bool
        PutBack(std::byte c) override {
            return this->FileStreamBase::PutBack(c);
        }
    };

    class OFileStream :
        public  OStream,
        public  __impl::FileStreamBase {
    protected:
        OFileStream() = default;

    public:
        OFileStream(std::string_view strvFilename) :
            FileStreamBase(strvFilename, "w") {}

        bool
        Write(std::byte c) override {
            return this->FileStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->FileStreamBase::WriteSome(buffer);
        }
    };

    class IOFileStream :
        public  IOStream,
        public  __impl::FileStreamBase {
    public:
        IOFileStream(std::string_view strvFilename) :
            FileStreamBase(strvFilename, "r+") {}

        bool
        Write(std::byte c) override {
            return this->FileStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->FileStreamBase::WriteSome(buffer);
        }

        std::optional<std::byte>
        Read() override {
            return this->FileStreamBase::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->FileStreamBase::ReadSome(buffer);
        }

        bool
        PutBack(std::byte c) override {
            return this->FileStreamBase::PutBack(c);
        }
    };
}

