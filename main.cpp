#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <docopt.h>
#include <nlohmann/json.hpp>
#include "src/backend.h"
#include "src/tanto.h"
#include "src/error.h"

#if defined(BACKEND_QT)
    #include "src/backends/qt/backendimpl.h"
#endif // defined(BACKEND_QT)

#if defined(BACKEND_GTK)
    #include "src/backends/gtk/backendimpl.h"
#endif // defined(BACKEND_GTK)

#if !defined(NDEBUG)
extern "C" const char* __lsan_default_options() { return "suppressions=leak.supp:print_suppressions=0"; }
#endif // !defined(NDEBUG)

template<> struct fmt::formatter<docopt::value> : ostream_formatter { };

namespace {

const std::unordered_map<std::string_view, std::string_view> BACKENDS{
#if defined(BACKEND_QT)
    {"qt", BackendQtImpl::version()},
#endif // defined(BACKEND_QT)
#if defined(BACKEND_GTK)
    {"gtk", BackendGtkImpl::version()},
#endif // defined(BACKEND_GTK)
};

std::string selected_backend{"gtk"};

const char USAGE[] =
R"(Tanto

    Usage:
        tanto stdin [--debug] [--backend=<name>]
        tanto load <filename> [--debug] [--backend=<name>]
        tanto message <title> <text> [(info|question|warning|error)] [--debug] [--backend=<name>]
        tanto confirm <title> <text> [(info|question|warning|error)] [--debug] [--backend=<name>]
        tanto input <title> [<text>] [<value>] [--debug] [--backend=<name>]
        tanto password <title> [<text>] [--debug] [--backend=<name>]
        tanto selectdir [<title>] [<dir>] [--debug] [--backend=<name>]
        tanto loadfile [<title>] [<filter>] [<dir>] [--debug] [--backend=<name>]
        tanto savefile [<title>] [<filter>] [<dir>] [--debug] [--backend=<name>]
        tanto list

    Options:
        -h --help            Show this screen.
        -v --version         Show version.
        -d --debug           Debug mode.
        -b --backend=<name>  Select backend.
)";

using BackendPtr = std::unique_ptr<Backend>;

[[nodiscard]] BackendPtr new_backend(const std::string& name, int argc, char* argv[])
{

#if defined(BACKEND_QT)
    if(name == "qt") return std::make_unique<BackendQtImpl>(argc, argv);
#endif // defined(BACKEND_QT)

#if defined(BACKEND_GTK)
    if(name == "gtk") return std::make_unique<BackendGtkImpl>(argc, argv);
#endif // defined(BACKEND_GTK)

    except("Backend '{}' not found", name);
}

tanto::FilterList parse_filter(const docopt::value& v)
{
    std::string filter = v ? v.asString() : std::string{};
    if(filter.empty()) return {};

    tanto::FilterList res;

    try {
        nlohmann::json filterobj = nlohmann::json::parse(filter);
        if(!filterobj.is_array()) except("Filter Error: expected 'array', got '{}'", filterobj.type_name());
        res = tanto::parse_filter(filterobj);
    }
    catch(nlohmann::json::parse_error& e) { 
        spdlog::critical("Filter Error: {}", e.what());
        std::exit(1);
    }

    return res;
}

std::string read_stdin()
{
    std::string input, line;

    while(std::getline(std::cin, line))
    {
        if(!input.empty()) input += "\n";
        input += line;
    }

    return input;
}

int execute_mode(const BackendPtr& backend, docopt::Options& options)
{
    if(options["list"].asBool())
    {
        for(const auto& [name, version] : BACKENDS)
            fmt::println("{}: {}", name, version);
    }
    else if(options["message"].asBool() || options["confirm"].asBool())
    {
        Backend::MessageIcon icon = Backend::MessageIcon::NONE;

        if(options["info"].asBool()) icon = Backend::MessageIcon::INFO;
        else if(options["question"].asBool()) icon = Backend::MessageIcon::QUESTION;
        else if(options["warning"].asBool()) icon = Backend::MessageIcon::WARNING;
        else if(options["error"].asBool()) icon = Backend::MessageIcon::ERROR;

        backend->message(options["<title>"].asString(), 
                         options["<text>"].asString(), 
                         options["confirm"].asBool() ? Backend::MessageType::CONFIRM : Backend::MessageType::MESSAGE,
                         icon);
    }
    else if(options["input"].asBool() || options["password"].asBool())
    {
        std::string text = options["<text>"] ? options["<text>"].asString() : std::string{};
        std::string value = options["<value>"] ? options["<value>"].asString() : std::string{};
        backend->input(options["<title>"].asString(), 
                       text,
                       value,
                       options["password"].asBool() ? Backend::InputType::PASSWORD : Backend::InputType::NORMAL);
    }
    else if(options["selectdir"].asBool())
    {
        backend->select_dir(options["<title>"] ? options["<title>"].asString() : std::string{},
                            options["<dir>"] ? options["<dir>"].asString() : std::string{});
    }
    else if(options["loadfile"].asBool())
    {
        backend->load_file(options["<title>"] ? options["<title>"].asString() : std::string{},
                           parse_filter(options["<filter>"]),
                           options["<dir>"] ? options["<dir>"].asString() : std::string{});
    }
    else if(options["savefile"].asBool())
    {
        backend->save_file(options["<title>"] ? options["<title>"].asString() : std::string{},
                           parse_filter(options["<filter>"]),
                           options["<dir>"] ? options["<dir>"].asString() : std::string{});
    }
    else 
        unreachable;

    return 0;
}

bool needs_json(docopt::Options& options) { return options["stdin"].asBool() || options["load"].asBool(); }

int execute_json(const BackendPtr& backend, docopt::Options& options)
{
    nlohmann::json jsonreq;

    try { 
        if(options["stdin"].asBool())
            jsonreq = nlohmann::json::parse(read_stdin());
        else if(options["load"].asBool()) {
            std::ifstream f{options["<filename>"].asString()};
            jsonreq = nlohmann::json::parse(f);
        }
        else
            unreachable;
    }
    catch(nlohmann::json::parse_error& e) {
        spdlog::critical(e.what());
    }

    auto window = tanto::parse(jsonreq);
    if(window) backend->process(*window);
    return backend->run();
}

} // namespace

int main(int argc, char* argv[])
{
    docopt::Options options = docopt::docopt(USAGE,
                                             {argv + 1, argv + argc},
                                             true,         // show help if requested
                                             "Tanto 1.0"); // version string

    if(options["--debug"].asBool())
    {
        for(auto const& arg : options)
            fmt::println("{} - {}", arg.first, arg.second);
    }

    if(!options["--backend"])
    {
        char* envbackend = std::getenv("TANTO_BACKEND");
        if(envbackend) selected_backend = envbackend;
    }
    else
        selected_backend = options["--backend"].asString();

    if(!BACKENDS.count(selected_backend))
    {
        fmt::println("ERROR: Unsupported backend '{}'", selected_backend);
        return 1;
    }

    BackendPtr backend = new_backend(selected_backend, argc, argv);

    if(needs_json(options)) return execute_json(backend, options);
    return execute_mode(backend, options);
}
