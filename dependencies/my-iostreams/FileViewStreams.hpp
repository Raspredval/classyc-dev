#pragma once
#include "CFile.hpp"

namespace io {
    namespace __impl {
        class FileViewStreamBase :
            virtual public      StreamState,
            virtual public      StreamPosition,
            protected           CFileView {
        public:
            FileViewStreamBase() = default;
            FileViewStreamBase(FILE* handle) :
                CFileView(handle) {}

            bool
            EndOfStream() const noexcept override {
                return this->CFileView::EndOfStream();
            }

            bool
            Good() const noexcept override {
                return !this->CFileView::Bad();
            }

            intptr_t
            GetPosition() const noexcept override {
                return this->CFileView::GetPosition();
            }

            bool
            SetPosition(intptr_t offset, StreamOffsetOrigin from = StreamOffsetOrigin::StreamStart) override {
                return this->CFileView::SetPosition(offset, from);
            }

            bool
            Flush() {
                return this->CFileView::Flush();
            }

            FILE*
            FileHandle() const noexcept {
                return this->CFileView::Handle();
            }
        };
    }

    class IFileViewStream :
        public              IStream,
        virtual public      __impl::FileViewStreamBase {
    protected:
        IFileViewStream() = default;

    public:
        IFileViewStream(FILE* handle) :
            FileViewStreamBase(handle) {}
        
        std::optional<std::byte>
        Read() override {
            return this->CFileView::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->CFileView::ReadSome(buffer);
        }
    };

    class OFileViewStream :
        public              OStream,
        virtual public      __impl::FileViewStreamBase {
    protected:
        OFileViewStream() = default;

    public:
        OFileViewStream(FILE* handle) :
            FileViewStreamBase(handle) {}
        
        bool
        Write(std::byte c) override {
            return this->CFileView::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->CFileView::WriteSome(buffer);
        }
    };

    class IOFileViewStream :
        public          IOStream,
        virtual public  __impl::FileViewStreamBase {
    public:
        IOFileViewStream(FILE* handle) :
            FileViewStreamBase(handle) {}

        std::optional<std::byte>
        Read() override {
            return this->CFileView::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->CFileView::ReadSome(buffer);
        }

        bool
        Write(std::byte c) override {
            return this->CFileView::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->CFileView::WriteSome(buffer);
        }
    };
}
