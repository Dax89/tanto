#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tanto::utils {

template<class... Ts>
struct Overload: Ts... {
    using Ts::operator()...;
};

#if __cplusplus < 202002L
template<class... Ts>
Overload(Ts...) -> Overload<Ts...>;
#endif // __cplusplus >= 202002L

template<typename>
constexpr bool always_false_v = false;

inline bool starts_with(const std::string& s, std::string_view what) {
    return s.find(what) == 0;
}

constexpr uint32_t fnv1a_32(std::string_view s) {
    uint32_t h = 2166136261U;

    for(char c : s) {
        h ^= c;
        h *= 16777619U;
    }

    return h;
}

namespace string_literals {

constexpr uint32_t operator"" _fnv1a_32(const char* s, size_t size) {
    return utils::fnv1a_32(std::string_view{s, size});
}
static_assert("0123456789ABCDEF"_fnv1a_32 == 0xe9cb8efd);

} // namespace string_literals

} // namespace tanto::utils
