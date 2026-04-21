#pragma once

#include <memory>

namespace Util {

template<typename T>
using Boxed = std::unique_ptr<T>;

// Вспомогательная функция для создания Boxed (аналог std::make_unique)
template<typename T, typename... Args>
Boxed<T> makeBoxed(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace Util