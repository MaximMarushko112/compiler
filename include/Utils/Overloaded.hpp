#pragma once

namespace Util {

template<typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// Дедукция для C++17 (можно использовать CTAD в C++20)
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace Util