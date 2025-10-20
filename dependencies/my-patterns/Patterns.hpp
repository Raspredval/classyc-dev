#pragma once
#include <map>
#include <array>
#include <memory>
#include <string>
#include <functional>

#include <my-iostreams/IOStreams.hpp>
#include <my-iostreams/Logging.hpp>

namespace patt {
    namespace __impl {
        class Pattern;
        class Grammar;
    }

    using Grammar   =
        __impl::Grammar;
    using Pattern   =
        std::shared_ptr<__impl::Pattern>;

    class Match {
    public:
        Match() :
            iBegin(0), iEnd(0) {}

        Match(intptr_t iBegin, intptr_t iEnd) :
            iBegin(iBegin), iEnd(iEnd) {}

        intptr_t
        Begin() const noexcept {
            return this->iBegin;
        }

        intptr_t
        End() const noexcept {
            return this->iEnd;
        }

        size_t
        Length() const noexcept {
            return (size_t)(this->iEnd - this->iBegin);
        }

        std::string
        GetString(io::IStream& is) const {
            intptr_t
                iCurrentPos = is.GetPosition();
            is.SetPosition(this->iBegin);
                
            std::string
                strMatch    = {};
            strMatch.resize(this->Length());
            
            is.ReadSome({
                (std::byte*)strMatch.data(),
                strMatch.size()
            });

            is.SetPosition(iCurrentPos);
            return strMatch;
        }

        auto&
        Concat(const Match& match) noexcept {
            this->iEnd  =
                match.iEnd;
            return *this;
        }

        class Error :
            public io::Error {
        public:
            Error() :
                io::Error("match failure") {}
        };

    private:
        intptr_t
            iBegin, iEnd;
    };

    namespace __impl {
        class Pattern {
        public:
            Pattern(const Pattern&) = default;
            Pattern()               = default;
            virtual ~Pattern()      = default;

            Match
            Eval(io::IStream& is) const {
                return (this->bNegated)
                    ? this->negEval(is)
                    : this->normEval(is);
            };

            [[nodiscard]]
            virtual patt::Pattern
            Clone() const = 0;

            // pattern inversion
            [[nodiscard]]
            friend patt::Pattern
            operator-(patt::Pattern&& pattern) {
                pattern->bNegated =
                    !pattern->bNegated;
                return pattern;
            }
        
        protected:
            virtual Match
            normEval(io::IStream& is) const = 0;

            virtual Match
            negEval(io::IStream& is) const {
                intptr_t
                    iBegin  = is.GetPosition();
                try {
                    this->normEval(is);
                }
                catch (const io::Error& err) {
                    intptr_t
                        iEnd    = is.GetPosition();
                    return Match{ iBegin, iEnd };
                }

                throw
                    Match::Error();
            }

            bool
                bNegated    = false;
        };

        // pattern inversion
        [[nodiscard]]
        inline patt::Pattern
        operator-(const patt::Pattern& pattern) {
            return -pattern->Clone();
        }

        class StringPattern :
            public Pattern {
        public:
            StringPattern(std::string_view strv) :
                str(strv) {}
            
            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<StringPattern>(*this);
            }

        private:
            Match
            normEval(io::IStream &is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
        
                for (char c : this->str) {
                    auto optc = is.Read();
                    if (!optc || (char)*optc != c)
                        throw Match::Error();
                }
        
                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }

            std::string
                str;
        };

        class ConcatPattern :
            public __impl::Pattern {
        public:
            ConcatPattern(const patt::Pattern& lhs, const patt::Pattern& rhs) :
                lhs(lhs), rhs(rhs) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<ConcatPattern>(*this);
            }
        
        private:
            Match
            normEval(io::IStream& is) const override {
                return this->lhs->Eval(is).Concat(
                    this->rhs->Eval(is));
            }

            patt::Pattern
                lhs, rhs;
        };

        // lhs followed by rhs
        [[nodiscard]] inline patt::Pattern
        operator>>(const patt::Pattern& lhs, const patt::Pattern& rhs) {
            return std::make_shared<ConcatPattern>(lhs, rhs);
        }

        class AnyPattern :
            public Pattern {
        public:
            AnyPattern() = default;

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<AnyPattern>(*this);
            }
        
        private:
            Match
            normEval(io::IStream& is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                
                if (!is.Read())
                    throw Match::Error();
                
                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }
        };

        class ChoicePattern :
            public  Pattern {
        public:
            ChoicePattern(const patt::Pattern& lhs, const patt::Pattern& rhs) :
                arrPatterns{lhs, rhs} {}
        
            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<ChoicePattern>(*this);
            }
        private:
            Match
            normEval(io::IStream& is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                
                for (const patt::Pattern& p : this->arrPatterns) {
                    try {
                        p->Eval(is);
                    }
                    catch(const Match::Error& err) {
                        is.SetPosition(iBegin);
                        continue;
                    }

                    intptr_t
                        iEnd    = is.GetPosition();
                    return Match{ iBegin, iEnd };
                }

                throw Match::Error();
            }

            std::array<patt::Pattern, 2>
                arrPatterns;
        };

        // ordered choice
        [[nodiscard]] inline patt::Pattern
        operator|=(const patt::Pattern& lhs, const patt::Pattern& rhs) {
            return std::make_shared<ChoicePattern>(lhs, rhs);
        }

        class HandlerPattern :
            public Pattern {
        public:
            using Callback =
                std::function<void(io::IStream&, const io::Expected<Match>&)>;

            template<typename Fn> requires
                std::is_constructible_v<Callback, Fn>
            HandlerPattern(const patt::Pattern& pattern, Fn&& fnCallback) :
                pattern     (pattern),
                fnCallback  (std::forward<Fn>(fnCallback)) {}
        
            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<HandlerPattern>(*this);
            }
        
        private:
            Match
            normEval(io::IStream& is) const override {
                io::Expected<Match>
                    var = Match{};
                try {
                    var = this->pattern->Eval(is);
                }
                catch (const Match::Error& exc) {
                    var = std::unexpected(exc);
                    this->fnCallback(is, var);
                    std::rethrow_exception(
                        std::current_exception());
                }

                this->fnCallback(is, var);
                return *var;
            }
        
            patt::Pattern
                pattern;
            Callback
                fnCallback;
        };

        // wrap pattern in "on pattern evaluation" event handler
        template<typename Fn> requires
            std::is_constructible_v<HandlerPattern::Callback, Fn>
        [[nodiscard]]
        inline patt::Pattern
        operator/(const patt::Pattern& pattern, Fn&& fn) {
            return std::make_shared<HandlerPattern>(
                pattern, fn);
        }

        class LocalePattern :
            public Pattern {
        public:
            using LocaleProc =
                int (&)(int);

            LocalePattern(LocaleProc lpfn) :
                lpfn(lpfn) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<LocalePattern>(*this);
            }
        
        private:
            Match
            normEval(io::IStream& is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                
                auto
                    optc    = is.Read();
                if (!optc || !this->lpfn((int)*optc))
                    throw Match::Error();

                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }

            LocaleProc
                lpfn;
        };

        class RepeatPattern :
            public Pattern {
        public:
            RepeatPattern(const patt::Pattern& pattern, size_t uCount) :
                pattern(pattern), uCount(uCount) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<RepeatPattern>(*this);
            }

        private:
            Match
            normEval(io::IStream& is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    this->pattern->Eval(is);
                }

                intptr_t
                    iCurr   = is.GetPosition();
                while (true) {
                    try {
                        this->pattern->Eval(is);
                        iCurr   = is.GetPosition();
                    }
                    catch(const Match::Error& err) {
                        is.SetPosition(iCurr);
                        break;
                    }
                }

                return Match{ iBegin, iCurr };
            }

            Match
            negEval(io::IStream& is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    intptr_t
                        iCurr   = is.GetPosition();
                    try {
                        this->pattern->Eval(is);
                    }
                    catch (const Match::Error& err) {
                        is.SetPosition(iCurr);
                        return Match{ iBegin, iCurr };
                    }
                }

                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }
        
            patt::Pattern
                pattern;
            size_t
                uCount;
        };

        // repeat pattern:
        //  if rhs is positive, repeat N+ times;
        //  if rhs is negative, repeat 0..N times.
        [[nodiscard]]
        inline patt::Pattern
        operator%(const patt::Pattern& lhs, intptr_t rhs) {
            patt::Pattern
                pattern = std::make_shared<RepeatPattern>(lhs, (size_t)std::abs(rhs));
            if (rhs < 0)
                pattern = -pattern;
            return pattern;
        }

        class RepeatExactPattern :
            public Pattern {
        public:
            RepeatExactPattern(const patt::Pattern& pattern, size_t uCount) :
                pattern(pattern), uCount(uCount) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<RepeatExactPattern>(*this);
            }
        
        private:
            Match
            normEval(io::IStream& is) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    this->pattern->Eval(is);
                }

                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }

            patt::Pattern
                pattern;
            size_t
                uCount;
        };

        // repeat pattern exactly N times
        [[nodiscard]]
        inline patt::Pattern
        operator*(const patt::Pattern& lhs, size_t rhs) {
            return std::make_shared<RepeatExactPattern>(lhs, rhs);
        }

        template<typename T>
        concept StringLike =
            std::constructible_from<std::string, T>;

        using MapPatterns =
            std::map<std::string, patt::Pattern>;

        class Grammar {
        private:
            class ConstAccessor {
            public:
                template<StringLike T>
                ConstAccessor(const std::shared_ptr<MapPatterns>& sptrPatterns, T&& key) :
                    strKey(std::forward<T>(key)),
                    sptrPatterns(sptrPatterns) {}

                [[nodiscard]]
                operator patt::Pattern() &&;

            protected:
                std::string
                    strKey;
                std::shared_ptr<MapPatterns>
                    sptrPatterns;
            };

            class Accessor :
                public ConstAccessor {
            public:
                template<StringLike T>
                Accessor(const std::shared_ptr<MapPatterns>& sptrPatterns, T&& key) :
                    ConstAccessor(sptrPatterns, std::forward<T>(key)) {}

                patt::Pattern&
                operator=(const patt::Pattern& pattern) && {
                    auto
                        result  = this->sptrPatterns->insert_or_assign(
                                    std::move(this->strKey), pattern);
                    if (!result.second)
                        throw io::Error("failed to insert a pattern into a grammar");
                    
                    return result.first->second;
                }
            };
            
        public:
            Grammar() :
                sptrPatterns(std::make_shared<MapPatterns>()) {}

            Grammar(const Grammar& obj) :
                sptrPatterns(obj.sptrPatterns) {}
            
            Grammar(Grammar&& obj) noexcept :
                sptrPatterns(std::move(obj.sptrPatterns)) {}

            auto&
            operator=(const Grammar& obj) {
                Grammar
                    temp    = obj;
                std::swap(
                    this->sptrPatterns,
                    temp.sptrPatterns);
                return *this;
            }

            auto&
            operator=(Grammar&& obj) noexcept {
                Grammar
                    temp    = std::move(obj);
                std::swap(
                    this->sptrPatterns,
                    temp.sptrPatterns);
                return *this;
            }

            template<StringLike T>
            ConstAccessor
            operator[](T&& key) const {
                return ConstAccessor(this->sptrPatterns, std::forward<T>(key));
            }

            template<StringLike T>
            Accessor
            operator[](T&& key) {
                return Accessor(this->sptrPatterns, std::forward<T>(key));
            }

        private:
            std::shared_ptr<MapPatterns>
                sptrPatterns;
        };

        class GrammarPattern :
            public Pattern {
        public:
            template<typename T> requires
                std::constructible_from<std::string, T>
            GrammarPattern(const std::shared_ptr<MapPatterns>& sptrPatterns, T&& key) :
                sptrPatterns(sptrPatterns), strKey(std::forward<T>(key)) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<GrammarPattern>(*this);
            }

        private:
            Match
            normEval(io::IStream& is) const override {
                return (*this->sptrPatterns)[this->strKey]->Eval(is);
            }

            std::shared_ptr<MapPatterns>
                sptrPatterns;
            std::string
                strKey;
        };

        inline Grammar::ConstAccessor::operator patt::Pattern() && {
            return std::make_shared<GrammarPattern>(
                this->sptrPatterns, std::move(this->strKey));
        }

        template<typename T, typename V>
        concept Appendable =
            requires (T& obj, std::decay_t<V> const& copy_ref) {
                { obj.push_back(copy_ref) };
            } ||
            requires (T& obj, std::decay_t<V>&& move_ref) {
                { obj.push_back(std::move(move_ref)) };
            };
        
        template<Appendable<std::string> T>
        class CapturePattern :
            public Pattern {
        public:
            CapturePattern(const patt::Pattern& pattern, T& refCaptures) :
                pattern(pattern),
                refCaptures(refCaptures) {}
        
            patt::Pattern
            Clone() const override {
                return std::make_shared<CapturePattern<T>>(*this);
            }

        private:
            Match
            normEval(io::IStream& is) const override {
                Match
                    m   = this->pattern->Eval(is);
                this->refCaptures.push_back(
                    m.GetString(is));
                return m;
            }

            patt::Pattern
                pattern;
            T&
                refCaptures;
        };

        template<Appendable<std::string> T>
        [[nodiscard]]
        inline patt::Pattern
        operator/(const patt::Pattern& pattern, T& refCaptures) {
            return std::make_shared<CapturePattern<T>>(pattern, refCaptures);
        }
    }

    [[nodiscard]]
    inline Pattern
    Str(std::string_view strv) {
        return std::make_shared<__impl::StringPattern>(
            strv);
    }

    [[nodiscard]]
    inline Pattern
    Any() {
        return std::make_shared<__impl::AnyPattern>();
    }

    [[nodiscard]]
    inline Pattern
    None() {
        return -Any();
    }

    [[nodiscard]]
    inline Pattern
    Endl() {
        return patt::Str("\n") |= patt::Str("\r\n") |= patt::None();
    }

    [[nodiscard]]
    inline Pattern
    Alpha() {
        return std::make_shared<__impl::LocalePattern>(isalpha);
    }

    [[nodiscard]]
    inline Pattern
    Alnum() {
        return std::make_shared<__impl::LocalePattern>(isalnum);
    }

    [[nodiscard]]
    inline Pattern
    Digit() {
        return std::make_shared<__impl::LocalePattern>(isdigit);
    }

    [[nodiscard]]
    inline Pattern
    HexDigit() {
        return std::make_shared<__impl::LocalePattern>(isxdigit);
    }

    [[nodiscard]]
    inline Pattern
    LowerCase() {
        return std::make_shared<__impl::LocalePattern>(islower);
    }

    [[nodiscard]]
    inline Pattern
    UpperCase() {
        return std::make_shared<__impl::LocalePattern>(isupper);
    }

    [[nodiscard]]
    inline Pattern
    Space() {
        return std::make_shared<__impl::LocalePattern>(isspace);
    }

    [[nodiscard]]
    inline Pattern
    Blank() {
        return std::make_shared<__impl::LocalePattern>(isblank);
    }

    [[nodiscard]]
    inline io::Expected<Match>
    SafeEval(const Pattern& p, io::IStream& is) noexcept {
        io::Expected<Match>
            var = Match{};
        try {
            var = p->Eval(is);
        }
        catch(const Match::Error& err) {
            var = std::unexpected(err);
        }

        return var;
    }

    [[nodiscard]]
    inline Match
    UnsafeEval(const Pattern& p, io::IStream& is) {
        return p->Eval(is);
    }
}
