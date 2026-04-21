#pragma once

#include <type_traits>
#include <variant>

namespace Util {

// Пустой кортеж
struct EmptyTuple {};

// Основной шаблон TTuple
template<typename... Ts>
struct TTuple {};

// Concat: объединение двух TTuple
template<typename, typename>
struct ConcatHelper;

template<typename... Ts, typename... Us>
struct ConcatHelper<TTuple<Ts...>, TTuple<Us...>> {
    using Result = TTuple<Ts..., Us...>;
};

template<typename A, typename B>
using Concat = typename ConcatHelper<A, B>::Result;

// Contains: проверяет, содержится ли тип T в TTuple
template<typename T, typename Tuple>
struct ContainsHelper;

template<typename T>
struct ContainsHelper<T, EmptyTuple> : std::false_type {};

template<typename T, typename First, typename... Rest>
struct ContainsHelper<T, TTuple<First, Rest...>>
    : std::conditional_t<std::is_same_v<T, First>,
                         std::true_type,
                         ContainsHelper<T, TTuple<Rest...>>> {};

template<typename T, typename Tuple>
inline constexpr bool Contains = ContainsHelper<T, Tuple>::value;

// TupleToVariant: преобразует TTuple в std::variant
template<typename Tuple>
struct TupleToVariantHelper;

template<typename... Ts>
struct TupleToVariantHelper<TTuple<Ts...>> {
    using Result = std::variant<Ts...>;
};

template<typename Tuple>
using TupleToVariant = typename TupleToVariantHelper<Tuple>::Result;

} // namespace Util