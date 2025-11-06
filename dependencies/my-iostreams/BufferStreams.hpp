#pragma once
#include <deque>
#include <cstdint>

#include "IOStreams.hpp"


namespace io {
    namespace __impl {
        class BufferStreamBase :
            virtual public  StreamState,
            virtual public  StreamPosition {
        public:
            BufferStreamBase() = default;
            BufferStreamBase(std::span<const std::byte> buffer) :
                deqBuffer(std::from_range, buffer) {}
            
            bool
            EndOfStream() const noexcept override {
                return this->flags_eof;
            }

            bool
            Good() const noexcept override {
                return true;
            }

            intptr_t
            GetPosition() const noexcept override {
                return this->iCurPos;
            }

            void
            ClearFlags() noexcept override {
                this->flags_eof = false;
            }

            bool
            SetPosition(intptr_t offset, StreamOffsetOrigin from = StreamOffsetOrigin::StreamStart) override {
                switch(from) {
                case StreamOffsetOrigin::CurrentPos:
                    offset  += this->iCurPos;
                    break;
                
                case StreamOffsetOrigin::StreamStart:
                    offset  += 0;
                    break;

                case StreamOffsetOrigin::StreamEnd:
                    offset  += (intptr_t)this->deqBuffer.size();
                    break;
                }

                auto
                    itNewPos    = this->deqBuffer.begin() + offset;
                if (itNewPos < this->deqBuffer.begin() || itNewPos >= this->deqBuffer.end())
                    return false;

                this->iCurPos       = offset;
                this->flags_eof     = false;
                this->retbuf_size   = 0;
                return true;
            }

            intptr_t
            Erase(intptr_t iFirst, intptr_t iLast) {
                return this->deqBuffer.erase(
                    this->deqBuffer.begin() + iFirst,
                    this->deqBuffer.begin() + iLast) - this->deqBuffer.begin();
                this->iCurPos   = std::min<intptr_t>(
                                    this->iCurPos,
                                    (intptr_t)this->deqBuffer.size());
            }

            intptr_t
            Insert(intptr_t iWhere, std::span<const std::byte> bytes) {
                auto
                    itWhere = this->deqBuffer.begin() + iWhere;
                this->deqBuffer.insert(
                    itWhere,
                    bytes.begin(),
                    bytes.end());
                return iWhere;
            }

            intptr_t
            Insert(intptr_t iWhere, io::SerialIStream& is, size_t uCount = SIZE_MAX) {
                auto
                    itWhere = this->deqBuffer.begin() + iWhere;
                for (size_t i = 0; i != uCount; ++i) {
                    std::optional<std::byte>
                        optc    = is.Read();
                    if (!optc)
                        break;

                    itWhere     = ++this->deqBuffer.insert(itWhere, *optc);
                }

                return iWhere;
            }

            intptr_t
            Replace(intptr_t iFirst, intptr_t iLast, std::span<const std::byte> bytes) {
                return this->Insert(
                    this->Erase(iFirst, iLast),
                    bytes);
            }

            intptr_t
            Replace(intptr_t iFirst, intptr_t iLast, io::SerialIStream& is, size_t uCount = SIZE_MAX) {
                return this->Insert(
                    this->Erase(iFirst, iLast),
                    is, uCount);
            }

            void
            ClearBuffer() {
                this->deqBuffer.clear();
                this->iCurPos   = 0;
                this->ClearFlags();
            }

        protected:
            bool
            Write(std::byte c) {
                auto
                    itCurPos    = this->deqBuffer.begin() + this->iCurPos;
                this->deqBuffer.insert(
                    itCurPos, c);
                this->iCurPos   += 1;

                this->retbuf_size = 0;
                this->ClearFlags();  
                return true;
            }

            size_t
            WriteSome(std::span<const std::byte> buffer) {
                auto
                    itCurPos    = this->deqBuffer.begin() + this->iCurPos;
                this->deqBuffer.insert_range(
                    itCurPos, buffer);
                this->iCurPos   += buffer.size();

                this->retbuf_size = 0;
                this->ClearFlags();
                return buffer.size();
            }

            std::optional<std::byte>
            Read() {
                if (this->retbuf_size != 0) {
                    this->retbuf_size -= 1;
                    return this->retbuf[this->retbuf_size];
                }

                auto
                    itCurr  = this->deqBuffer.begin() + this->iCurPos,
                    itEnd   = this->deqBuffer.end();
                if (itCurr != itEnd) {
                    this->iCurPos   += 1;
                    return *itCurr;
                }

                this->flags_eof = true;
                return std::nullopt;
            }

            size_t
            ReadSome(std::span<std::byte> buffer) {
                for (size_t i = 0; i != buffer.size(); ++i) {
                    std::optional<std::byte>
                        c   = this->Read();
                    if (!c.has_value()) {
                        return i;
                    }

                    buffer[i] = *c;
                }

                return buffer.size();
            }

            bool
            PutBack(std::byte c) {
                if (this->retbuf_size < sizeof(this->retbuf)) {
                    this->retbuf[this->retbuf_size] = c;
                    this->retbuf_size += 1;
                    this->ClearFlags();
                    return true;
                }
                else
                    return false;
            }
    
        private:
            std::deque<std::byte>
                deqBuffer;
            intptr_t
                iCurPos = 0;
            
            struct {
                std::byte
                    retbuf[alignof(intptr_t) - 1];
                uint8_t
                    retbuf_size : 7 = 0,
                    flags_eof   : 1 = false;
            };
        };
    }

    class IBufferStream :
        public          IStream,
        virtual public  __impl::BufferStreamBase {
    protected:
        IBufferStream() = default;

    public:
        IBufferStream(std::span<const std::byte> buffer) :
            BufferStreamBase(buffer) {}

        std::optional<std::byte>
        Read() override {
            return this->BufferStreamBase::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->BufferStreamBase::ReadSome(buffer);
        }

        bool
        PutBack(std::byte c) override {
            return this->BufferStreamBase::PutBack(c);
        }
    };

    class OBufferStream :
        public          OStream,
        virtual public  __impl::BufferStreamBase {
    public:
        OBufferStream() = default;
        
        bool
        Write(std::byte c) override {
            return this->BufferStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->BufferStreamBase::WriteSome(buffer);
        }
    };

    class IOBufferStream :
        public          IOStream,
        virtual public  __impl::BufferStreamBase {
    public:
        IOBufferStream() = default;
        IOBufferStream(std::span<const std::byte> buffer) :
            BufferStreamBase(buffer) {}
        
        std::optional<std::byte>
        Read() override {
            return this->BufferStreamBase::Read();
        }

        size_t
        ReadSome(std::span<std::byte> buffer) override {
            return this->BufferStreamBase::ReadSome(buffer);
        }

        bool
        PutBack(std::byte c) override {
            return this->BufferStreamBase::PutBack(c);
        }

        bool
        Write(std::byte c) override {
            return this->BufferStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->BufferStreamBase::WriteSome(buffer);
        }
    };
}

