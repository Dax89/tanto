#include "tanto.h"
#include "error.h"
#include "utils.h"
#include <cctype>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string_view>

#if defined(__unix__)
    #include <curl/curl.h>
    #include <unistd.h>
#elif defined(_WIN32)
    #include <urlmon.h>
#endif

namespace {

std::vector<char> g_tmpnamebuffer;

#if defined(__unix__)
size_t curl_write(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ofs = reinterpret_cast<std::fstream*>(userdata);
    size_t nbytes = size * nmemb;
    ofs->write(reinterpret_cast<char*>(ptr), nbytes);
    return nbytes;
}
#endif

template<typename Function>
void split_each(std::string_view s, char sep, Function f) {
    size_t i = 0, start = 0;

    for(; i < s.size(); i++) {
        if(std::isspace(s[i]))
            continue;
        if(s[i] != sep)
            continue;
        if(i > start)
            f(s.substr(start, i - start));
        start = i + 1;
    }

    while(start < s.size() && std::isspace(s[start]))
        start++;
    if(start < s.size())
        f(s.substr(start));
}

[[nodiscard]] std::vector<std::string> parse_ext(std::string_view filter) {
    std::vector<std::string> ext;
    split_each(filter, ';', [&](std::string_view s) { ext.emplace_back(s); });
    return ext;
}

} // namespace

namespace tanto {

namespace fs = std::filesystem;

Header parse_header(const types::Widget& w) {
    Header header;
    nlohmann::json rawheader = w.prop<nlohmann::json::array_t>("header");

    for(const auto& h : rawheader) {
        if(h.is_string())
            header.emplace_back(HeaderItem{h, h});
        else
            header.push_back(h.get<HeaderItem>());
    }

    return header;
}

FilterList parse_filter(std::string_view filter) {
    FilterList filters;
    Filter f;

    split_each(filter, '|', [&](std::string_view s) {
        if(!f.name.empty()) {
            f.ext = parse_ext(s);
            filters.push_back(f);
            f = Filter{};
        }
        else
            f.name = std::string{s};
    });

    return filters;
}

std::optional<types::Window> parse(const nlohmann::json& jsonreq) {
    using namespace tanto::utils::string_literals;

    if(jsonreq.is_null())
        return std::nullopt;
    assume(jsonreq.is_object());

    types::Window window = jsonreq.get<types::Window>();

    switch(utils::fnv1a_32(window.type)) {
        case "window"_fnv1a_32:
            // case "popup"_fnv1a_32:
            // case "tool"_fnv1a_32:
            break;

        default: except("Invalid type: '{}'", window.type); break;
    }

    return window;
}

std::optional<std::pair<std::string, int>> parse_font(const std::string& font) {
    if(font.empty())
        return std::nullopt;

    std::string name;
    int size = -1;

    auto it = font.begin();

    if(*it == '\'') {
        bool complete = false;
        auto startit = ++it;

        for(auto iit = startit; iit != font.end(); iit++) {
            if(*iit == '\'') {
                name = std::string{startit, iit};
                it = ++iit;
                complete = true;
                break;
            }
        }

        if(!complete)
            return std::nullopt;
    }
    else {
        auto iit = it, endit = font.end();

        for(; iit != font.end(); iit++) {
            if(*iit == ' ') {
                endit = iit;
                break;
            }
        }

        name = std::string{it, endit};
        it = endit;
    }

    while(it != font.end() && std::isspace(*it))
        ++it;

    if(it != font.end()) {
        int startidx = std::distance(font.begin(), it);
        auto res = std::from_chars(font.data() + startidx,
                                   font.data() + font.size(), size);
        if(res.ec != std::errc{} || res.ptr < (font.data() + font.size()))
            return std::nullopt;
    }

    return std::make_optional(std::make_pair(name, size));
}

std::string download_file(const std::string& url) {
    if(url.empty())
        return std::string{};
    if(!utils::starts_with(url, "https://") &&
       !tanto::utils::starts_with(url, "http://"))
        return url;

    g_tmpnamebuffer.resize(TMP_MAX);
    std::tmpnam(g_tmpnamebuffer.data());
    std::string filepath =
        (fs::temp_directory_path() / std::string{g_tmpnamebuffer.data()})
            .string();

#if defined(__unix__)
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_write);

    std::fstream ofs{filepath, std::ios::out | std::ios::binary};
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &ofs);
    curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);
    curl_global_init(CURL_GLOBAL_ALL);
#elif defined(_WIN32)
    // https://stackoverflow.com/questions/66389908/download-url-file-from-direct-download-url-c
    #error "Windows is not supported (yet)"
#endif

    return filepath;
}

std::string stringify(const nlohmann::json& arg) {
    if(arg.is_string())
        return arg.get<std::string>();
    if(arg.is_null())
        return std::string{};
    if(arg.is_primitive())
        return arg.dump();

    except("Cannot stringify type: '{}'", arg.type_name());
}

} // namespace tanto
