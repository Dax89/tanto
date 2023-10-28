#include "types.h"
#include "utils.h"
#include "error.h"
#include "unordered_set"

#define JSON_FIELD_T(x) {#x, w.x}
#define JSON_FIELD_F(x) w.x = j.value(#x, w.x)

namespace {

const std::unordered_set<std::string_view> WIDGET_BUILTINS = {
    "id", "type", "title", "group",
    "text", "value",
    "width", "height",
    "enabled", "fill",
    "items", "properties"
};

} // namespace

namespace tanto::types {

void to_json(nlohmann::json& j, const Widget& w)
{
    j = nlohmann::json{
        JSON_FIELD_T(id), JSON_FIELD_T(type), JSON_FIELD_T(title), JSON_FIELD_T(group),
        JSON_FIELD_T(text), JSON_FIELD_T(value), 
        JSON_FIELD_T(enabled), JSON_FIELD_T(fill), JSON_FIELD_T(width), JSON_FIELD_T(height),
        {"items", nlohmann::json::array_t{}},
    };

    for(const auto& [k, v] : w.properties)
    {
        if(WIDGET_BUILTINS.count(k)) continue;
        j[k] = v;
    }

    for(auto c : w.items)
    {
        nlohmann::json obj;

        std::visit(tanto::utils::Overload{
                [&obj](Widget& arg) { tanto::types::to_json(obj, arg); },
                [&obj](std::string& arg) { obj = arg; }
        }, c);

        j["items"].push_back(obj);
    }
}

void from_json(const nlohmann::json& j, Widget& w)
{
    JSON_FIELD_F(id); JSON_FIELD_F(type); JSON_FIELD_F(title); JSON_FIELD_F(group);
    JSON_FIELD_F(text); JSON_FIELD_F(value); 
    JSON_FIELD_F(enabled); JSON_FIELD_F(fill);
    JSON_FIELD_F(width); JSON_FIELD_F(height);

    for(const auto& [k, v] : j.items())
    {
        if(WIDGET_BUILTINS.count(k)) continue;
        w.properties[k] = v;
    }

    for(const auto& c : j.value("items", nlohmann::json::array_t{}))
    {
        if(c.is_object())
        {
            Widget obj;
            tanto::types::from_json(c, obj);
            w.items.emplace_back(obj);
        }
        else if(c.is_string())
            w.items.emplace_back(c.get<std::string>());
        else
            except("Type {} is not supported", c.type_name());
    }
}

} // namespace tanto::types
