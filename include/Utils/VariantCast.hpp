#pragma once

#include <variant>
#include <type_traits>

namespace Util {

namespace detail {
    template<typename TargetVariant, typename... SourceTypes>
    TargetVariant variant_cast_impl(const std::variant<SourceTypes...>& src) {
        return std::visit(
            [](const auto& val) -> TargetVariant {
                return TargetVariant{val};
            },
            src
        );
    }
} // namespace detail

// Преобразование variant к другому variant (типы источника должны быть подмножеством целевых)
template<typename TargetVariant, typename... SourceTypes>
TargetVariant variant_cast(const std::variant<SourceTypes...>& src) {
    return detail::variant_cast_impl<TargetVariant>(src);
}

template<typename TargetVariant, typename... SourceTypes>
TargetVariant variant_cast(std::variant<SourceTypes...>&& src) {
    return std::visit(
        [](auto&& val) -> TargetVariant {
            return TargetVariant{std::forward<decltype(val)>(val)};
        },
        std::move(src)
    );
}

} // namespace Util