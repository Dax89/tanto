#include "tanto.h"
#include <cstddef>
#include <filesystem>
#include <charconv>
#include <fstream>
#include "error.h"
#include "utils.h"

#if defined(__unix__)
    #include <unistd.h>
    #include <curl/curl.h>
#elif defined(_WIN32)
    #include <urlmon.h>
#endif

namespace  {

std::vector<char> g_tmpnamebuffer;

#if defined(__unix__)
size_t curl_write(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    std::fstream* ofs = reinterpret_cast<std::fstream*>(userdata);
    size_t nbytes = size * nmemb;
    ofs->write(reinterpret_cast<char*>(ptr), nbytes);
    return nbytes;
}
#endif

} // namespace

namespace tanto {

namespace fs = std::filesystem;

FilterList parse_filter(const nlohmann::json& filters)
{
    assume(filters.is_array());

    FilterList f;

    for(const auto& item : filters)
        f.push_back(item.get<Filter>());

    return f;
}

std::optional<types::Window> parse(const nlohmann::json& jsonreq)
{
    using namespace tanto::utils::string_literals;

    if(jsonreq.is_null()) return std::nullopt;
    assume(jsonreq.is_object());

    types::Window window = jsonreq.get<types::Window>();

    switch(utils::fnv1a_32(window.type))
    {
        case "window"_fnv1a_32:
        case "popup"_fnv1a_32:
        case "tool"_fnv1a_32:
            break;

        default:
            except("Invalid type: '{}'", window.type);
            break;
    }

    return window;
}

std::optional<std::pair<std::string, int>> parse_font(const std::string& font)
{
    if(font.empty()) return std::nullopt;

    std::string name;
    int size = -1;

    auto it = font.begin();

    if(*it == '\'')
    {
        bool complete = false;
        auto startit = ++it;

        for(auto iit = startit; iit != font.end(); iit++)
        {
            if(*iit == '\'')
            {
                name = std::string{startit, iit};
                it = ++iit;
                complete = true;
                break;
            }
        }

        if(!complete) return std::nullopt;
    }
    else
    {
        auto iit = it, endit = font.end();

        for( ; iit != font.end(); iit++)
        {
            if(*iit == ' ')
            {
                endit = iit;
                break;
            }
        }

        name = std::string{it, endit};
        it = endit;
    }

    while(it != font.end() && std::isspace(*it)) ++it;

    if(it != font.end())
    {
        int startidx = std::distance(font.begin(), it);
        auto res = std::from_chars(font.data() + startidx, font.data() + font.size(), size);
        if(res.ec != std::errc{} || res.ptr < (font.data() + font.size())) return std::nullopt;
    }

    return std::make_optional(std::make_pair(name, size));
}

std::string download_file(const std::string& url)
{
    if(url.empty()) return std::string{};
    if(!utils::starts_with(url, "https://") && !tanto::utils::starts_with(url, "http://")) return url;

    g_tmpnamebuffer.resize(TMP_MAX);
    std::tmpnam(g_tmpnamebuffer.data());
    std::string filepath = (fs::temp_directory_path() / std::string{g_tmpnamebuffer.data()}).string();

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

} // namespace tanto
