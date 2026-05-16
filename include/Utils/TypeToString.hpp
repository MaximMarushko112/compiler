#pragma once

#include <string>
#include <typeinfo>
#include <cstdlib>
#include <memory>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace Util {

inline std::string demangle(const char* name) {
#ifdef __GNUG__
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> res{
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
#else
    return name;
#endif
}

template<typename T>
std::string toStringUnqualified() {
    std::string full = demangle(typeid(T).name());
    // Удаляем namespace, если есть
    size_t last_colon = full.rfind("::");
    if (last_colon != std::string::npos) {
        return full.substr(last_colon + 2);
    }
    return full;
}

} // namespace Util