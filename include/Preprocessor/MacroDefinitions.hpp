#pragma once
#include <map>
#include <memory>
#include <string>

#include "Macro.hpp"


namespace Preprocessor {
    namespace __impl {
        class MacroDefinitions;
    }

    using MacroDefinitions =
        std::shared_ptr<__impl::MacroDefinitions>;

    namespace __impl {
        namespace PP =
            ::Preprocessor;

        class MacroDefinitions {
        public:
            bool
            Insert(const std::string& strKey, const PP::Macro& macro) {
                auto
                    result  = this->mapMacros.emplace(
                                strKey, macro);
                return result.second;
            }

            bool
            HasParent() const noexcept {
                return this->sptrParent != nullptr;
            }

            const PP::MacroDefinitions&
            Parent() const noexcept {
                return this->sptrParent;
            }

            PP::MacroDefinitions&
            Parent() noexcept {
                return this->sptrParent;
            }

            PP::Macro
            GetMacro(const std::string& strKey) const {
                auto
                    it  = this->mapMacros.find(strKey);
                if (it != this->mapMacros.end())
                    return it->second;
                else
                    return (this->HasParent())
                        ? this->sptrParent->GetMacro(strKey)
                        : nullptr;
            }

            PP::Macro
            operator[](const std::string& strKey) const {
                return this->GetMacro(strKey);
            }

        private:
            std::map<std::string, PP::Macro>
                mapMacros;
            PP::MacroDefinitions
                sptrParent;
        };
    }
}