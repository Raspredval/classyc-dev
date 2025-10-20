#pragma once
#include <cstdio>
#include <cstdint>
#include <string_view>

#include <IOStreams.hpp>
#include <Logging.hpp>

namespace io {
    class CFileView {
    public:
        CFileView() = default;

        CFileView(FILE* handle) :
            handle(handle) {}

        bool
        EndOfStream() const noexcept {
            return feof(this->handle);
        }

        bool
        Bad() const noexcept {
            return ferror(this->handle);
        }

        int64_t
        GetPosition() const noexcept {
            return ftell(this->handle);
        }

        bool
        SetPosition(
            int64_t             offset,
            StreamOffsetOrigin  from = StreamOffsetOrigin::StreamStart)
        {
            return !fseek(
                this->handle,
                offset,
                (int)from);
        }

        FILE*
        Handle() const noexcept {
            return this->handle;
        }

        bool
        Flush() {
            return !fflush(this->handle);
        }

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

    protected:
        FILE*
            handle = nullptr;
    };

    class CFile :
        public CFileView {
    private:
        CFile(FILE* handle) noexcept :
            CFileView(handle) {}

    public:
        CFile() = default;
        CFile(const CFile&) = delete;
        
        CFile(CFile&& obj) noexcept {
            this->handle    = obj.handle;
            obj.handle      = nullptr;
        }

        auto&
        operator=(CFile&& obj) noexcept {
            CFile
                temp    = std::move(obj);
            std::swap(
                this->handle, temp.handle);
            return *this;
        }

        CFile(std::string_view strvFilename, std::string_view strvMode) {
            if (!this->Open(strvFilename, strvMode)) {
                throw io::Error(
                    "failed to open file {} with mode {}",
                    strvFilename, strvMode);
            }
        }

        ~CFile() noexcept {
            if (this->handle != nullptr)
                fclose(this->handle);
        }

        bool
        Open(std::string_view strvFilename, std::string_view strvMode) noexcept {
            CFile
                temp    = CFile(fopen(
                            strvFilename.data(),
                            strvMode.data()));
            if (temp.handle == nullptr)
                return false;
            
            std::swap(
                this->handle, temp.handle);
            return true;
        }
    };
}