#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tanto::utils {

template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
template<class... Ts> Overload(Ts...) -> Overload<Ts...>; // line not needed in C++20...

template<typename> constexpr bool always_false_v = false;

inline bool starts_with(const std::string& s, std::string_view what) { return s.find(what) == 0; }

constexpr uint32_t fnv1a_32(std::string_view s) {
    uint32_t h = 2166136261u;

    for(char c : s) {
        h ^= c;
        h *= 16777619u;
    }

    return h;
}

namespace string_literals {

constexpr uint32_t operator"" _fnv1a_32(const char* s, size_t size) { return utils::fnv1a_32(std::string_view{s, size}); }
static_assert("0123456789ABCDEF"_fnv1a_32 == 0xe9cb8efd);

} // namespace string_literals

} // namespace tanto::utils
