#pragma once
#include <format>
#include <expected>
#include <stdexcept>

namespace io {
    class Error :
        public std::runtime_error {
    public:
        template<typename... Args>
        Error(const std::format_string<Args...> strfmt, Args&&... args) :
            std::runtime_error(std::format(strfmt, std::forward<Args>(args)...)) {}
    };

    template<typename T>
    using Expected =
        std::expected<T, Error>;
}
