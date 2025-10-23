#pragma once
#include <map>
#include <memory>
#include <string>
#include <string_view>

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
            Insert(std::string_view strvKey, const PP::Macro& macro) {
                auto
                    result  = this->mapMacros.emplace(
                                strvKey, macro);
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
            GetMacro(std::string_view strvKey) const {

            }

            PP::Macro
            operator[](std::string_view strvKey) const {
                return this->GetMacro(strvKey);
            }

        private:
            std::map<std::string, PP::Macro>
                mapMacros;
            PP::MacroDefinitions
                sptrParent;
        };
    }
}