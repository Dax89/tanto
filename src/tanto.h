#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include "types.h"

namespace tanto {

constexpr int NUMBER_MIN = 0;
constexpr int NUMBER_MAX = 99;

struct Filter {
    std::string name;
    std::vector<std::string> ext;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Filter, name, ext);
};

using FilterList = std::vector<Filter>;

FilterList parse_filter(const nlohmann::json& filters);
std::optional<types::Window> parse(const nlohmann::json& jsonreq);
std::optional<std::pair<std::string, int>> parse_font(const std::string& font);
std::string download_file(const std::string& url);

} // namespace tanto
