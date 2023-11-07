#pragma once

#include "types.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tanto {

constexpr int NUMBER_MIN = 0;
constexpr int NUMBER_MAX = 99;

struct HeaderItem {
    std::string id;
    std::string text;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(HeaderItem, id, text);
};

using Header = std::vector<HeaderItem>;

struct Filter {
    std::string name;
    std::vector<std::string> ext;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Filter, name, ext);
};

using FilterList = std::vector<Filter>;

Header parse_header(const types::Widget& w);
FilterList parse_filter(std::string_view filter);
std::optional<types::Window> parse(const nlohmann::json& jsonreq);
std::optional<std::pair<std::string, int>> parse_font(const std::string& font);
std::string download_file(const std::string& url);
std::string stringify(const nlohmann::json& arg);

} // namespace tanto
