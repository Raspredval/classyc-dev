#pragma once
#include <deque>
#include <cstdint>

#include "IOStreams.hpp"


namespace io {
    namespace __impl {
        template<typename I, typename T>
        concept MutableIterator =
            requires(I it, T& val, const T& cval) {
                { val = *it  };
                { *it = cval };
            } &&
            std::three_way_comparable<I>;

        template<typename I, typename T>
        concept ConstIterator =
            requires(I it, T& val) {
                { val = *it };
            } &&
            std::three_way_comparable<I>;

        template<typename R, typename T>
        concept ConstRange =
            requires(R rng) {
                { rng.begin() } -> ConstIterator<T>;
                { rng.end()   } -> ConstIterator<T>;
            };

        template<typename R, typename T>
        concept MutableRange =
            requires(R rng) {
                { rng.begin() } -> MutableIterator<T>;
                { rng.end()   } -> MutableIterator<T>;
            };

        class BufferStreamBase :
            virtual public  StreamState,
            virtual public  StreamPosition {
        public:
            BufferStreamBase() = default;
            BufferStreamBase(std::span<const std::byte> buffer) :
                deqBuffer(std::from_range, buffer) {}
            
            bool
            EndOfStream() const noexcept override {
                auto
                    itCur   = this->deqBuffer.begin() + this->iCurPos;
                return itCur >= this->deqBuffer.end();
            }

            bool
            Good() const noexcept override {
                return true;
            }

            intptr_t
            GetPosition() const noexcept override {
                return this->iCurPos;
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

                this->iCurPos   = offset;
                return true;
            }
            
            template<ConstIterator<std::byte> I>
            intptr_t
            Insert(intptr_t iWhere, I itBegin, I itEnd) {
                return this->deqBuffer.insert(
                    this->deqBuffer.begin() + iWhere,
                    itBegin, itEnd) - this->deqBuffer.begin();
            }

            template<ConstRange<std::byte> R>
            intptr_t
            Insert(intptr_t iWhere, R&& bytes) {
                return this->Insert(iWhere,
                    std::forward<R>(bytes).begin(),
                    std::forward<R>(bytes).end());
            }

            intptr_t
            Erase(intptr_t iFirst, intptr_t iLast) {
                return this->deqBuffer.erase(
                    this->deqBuffer.begin() + iFirst,
                    this->deqBuffer.begin() + iLast) - this->deqBuffer.begin();
            }

            template<ConstIterator<std::byte> I>
            intptr_t
            Replace(intptr_t iFirst, intptr_t iLast, I itBegin, I itEnd) {
                return this->Insert(
                    this->Erase(iFirst, iLast),
                    itBegin, itEnd);
            }

            template<ConstRange<std::byte> R>
            intptr_t
            Replace(intptr_t iFirst, intptr_t iLast, R&& bytes) {
                return this->Replace(
                    iFirst, iLast,
                    std::forward<R>(bytes).begin(),
                    std::forward<R>(bytes).end());
            }

        protected:
            bool
            Write(std::byte c) {
                auto
                    itCurPos    = this->deqBuffer.begin() + this->iCurPos;
                this->deqBuffer.insert(
                    itCurPos, c);
                this->iCurPos   += 1;

                return true;
            }

            size_t
            WriteSome(std::span<const std::byte> buffer) {
                auto
                    itCurPos    = this->deqBuffer.begin() + this->iCurPos;
                this->deqBuffer.insert_range(
                    itCurPos, buffer);
                this->iCurPos   += buffer.size();

                return buffer.size();
            }

            std::optional<std::byte>
            Read() {
                if (this->EndOfStream())
                    return std::nullopt;

                return *(this->deqBuffer.begin() + this->iCurPos++);
            }

            size_t
            ReadSome(std::span<std::byte> buffer) {
                for (size_t i = 0; i != buffer.size(); ++i) {
                    std::optional<std::byte>
                        c   = this->Read();
                    if (!c.has_value())
                        return i;

                    buffer[i] = *c;
                }

                return buffer.size();
            }

        private:
            std::deque<std::byte>
                deqBuffer;
            int64_t
                iCurPos = 0;
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
        Write(std::byte c) override {
            return this->BufferStreamBase::Write(c);
        }

        size_t
        WriteSome(std::span<const std::byte> buffer) override {
            return this->BufferStreamBase::WriteSome(buffer);
        }
    };
}

