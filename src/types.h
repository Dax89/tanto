#pragma once

#include <string>
#include <vector>
#include <variant>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <nlohmann/json.hpp>

namespace tanto::types {

struct Widget;

using MultiValue = std::variant<Widget, std::string>;
using MultiValueList = std::vector<MultiValue>;
using Properties = std::map<std::string, nlohmann::json, std::less<>>;

struct Widget
{
    // Base
    bool enabled{true}, fill{false}; 
    std::string id, type, title, group;

    // Values
    std::string text;
    int value{0};
    
    int width{0}, height{0}; // Metrics
    MultiValueList items;    // Container

    Properties properties;

    Widget() = default;
    explicit Widget(const std::string& t): type{t} { }
    inline bool has_group() const { return !group.empty(); }
    inline bool has_id() const { return !id.empty(); }
    inline bool has_prop(std::string_view key) const { return properties.count(key); }
    inline const std::string& get_id() const { return id.empty() ? text : id; }
    inline operator bool() const { return !type.empty(); }

    template<typename T>
    inline T prop(std::string_view key, T fallback = T{}) const {
        auto it = properties.find(key);
        if(it == properties.end()) return fallback;

        if constexpr(std::is_same_v<T, nlohmann::json>) return it->second;
        else return it->second.get<T>();
    }

    friend void to_json(nlohmann::json& j, const Widget& w);
    friend void from_json(const nlohmann::json& j, Widget& w);
};

struct Window {
    std::string type, title, font;
    int x{}, y{}, width{}, height{};
    bool fixed{false};
    Widget body;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Window, type, title, font, x, y, width, height, fixed, body)
};

} // namespace tanto::types
