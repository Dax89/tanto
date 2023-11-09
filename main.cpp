#include "src/backend.h"
#include "src/error.h"
#include "src/tanto.h"
#include <algorithm>
#include <cl/cl.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#if defined(BACKEND_QT)
    #include "src/backends/qt/backendimpl.h"
#endif

#if defined(BACKEND_GTK)
    #include "src/backends/gtk/backendimpl.h"
#endif

#if !defined(NDEBUG)
extern "C" const char* __lsan_default_options() { // NOLINT
    return "suppressions=leak.supp:print_suppressions=0";
} // NOLINT
#endif

namespace {

const std::vector<std::pair<std::string_view, std::string_view>> BACKENDS {
#if defined(BACKEND_GTK)
    {"gtk", BackendGtkImpl::version()},
#endif
#if defined(BACKEND_QT)
        {"qt", BackendQtImpl::version()},
#endif
};

std::string selectedbackend;

using BackendPtr = std::unique_ptr<Backend>;

[[nodiscard]] BackendPtr new_backend(const std::string& name, int argc,
                                     char** argv) {
#if defined(BACKEND_GTK)
    if(name == "gtk")
        return std::make_unique<BackendGtkImpl>(argc, argv);
#endif // defined(BACKEND_GTK)

#if defined(BACKEND_QT)
    if(name == "qt")
        return std::make_unique<BackendQtImpl>(argc, argv);
#endif // defined(BACKEND_QT)

    except("Backend '{}' not found", name);
}

bool has_backend(std::string_view n) {
    return std::find_if(BACKENDS.begin(), BACKENDS.end(), [n](const auto& x) {
               return x.first == n;
           }) != BACKENDS.end();
}

tanto::FilterList parse_filter(const cl::Value& arg) {
    return tanto::parse_filter(arg ? arg.to_stringview() : std::string_view{});
}

std::string read_stdin() {
    std::string input, line;

    while(std::getline(std::cin, line)) {
        if(!input.empty())
            input += "\n";
        input += line;
    }

    return input;
}

int execute_mode(const BackendPtr& backend, cl::Values& args) {
    if(args["message"].to_bool() || args["confirm"].to_bool()) {
        Backend::MessageIcon icon = Backend::MessageIcon::NONE;

        if(args["info"].to_bool())
            icon = Backend::MessageIcon::INFO;
        else if(args["question"].to_bool())
            icon = Backend::MessageIcon::QUESTION;
        else if(args["warning"].to_bool())
            icon = Backend::MessageIcon::WARNING;
        else if(args["error"].to_bool())
            icon = Backend::MessageIcon::ERROR;

        backend->message(args["title"].to_string(), args["text"].to_string(),
                         args["confirm"].to_bool()
                             ? Backend::MessageType::CONFIRM
                             : Backend::MessageType::MESSAGE,
                         icon);
    }
    else if(args["input"].to_bool() || args["password"].to_bool()) {
        std::string text =
            args["text"] ? args["text"].to_string() : std::string{};
        std::string value =
            args["value"] ? args["value"].to_string() : std::string{};
        backend->input(args["title"].to_string(), text, value,
                       args["password"].to_bool() ? Backend::InputType::PASSWORD
                                                  : Backend::InputType::NORMAL);
    }
    else if(args["selectdir"].to_bool()) {
        backend->select_dir(
            args["title>"] ? args["title"].to_string() : std::string{},
            args["dir>"] ? args["dir"].to_string() : std::string{});
    }
    else if(args["loadfile"].to_bool()) {
        backend->load_file(
            args["title>"] ? args["title"].to_string() : std::string{},
            parse_filter(args["filter"]),
            args["dir>"] ? args["dir"].to_string() : std::string{});
    }
    else if(args["savefile"].to_bool()) {
        backend->save_file(
            args["title"] ? args["title"].to_string() : std::string{},
            parse_filter(args["filter"]),
            args["dir"] ? args["dir"].to_string() : std::string{});
    }
    else
        unreachable;

    return 0;
}

bool needs_json(cl::Values& values) {
    return values["stdin"].to_bool() || values["load"].to_bool();
}

int execute_json(const BackendPtr& backend, cl::Values& args) {
    nlohmann::json jsonreq;

    try {
        if(args["stdin"].to_bool())
            jsonreq = nlohmann::json::parse(read_stdin());
        else if(args["load"].to_bool()) {
            std::ifstream f{std::string{args["filename"].to_string()}};
            jsonreq = nlohmann::json::parse(f);
        }
        else
            unreachable;
    }
    catch(nlohmann::json::parse_error& e) {
        spdlog::critical(e.what());
    }

    auto window = tanto::parse(jsonreq);
    if(window)
        backend->process(*window);
    return backend->run();
}

} // namespace

int main(int argc, char** argv) {
    using namespace cl::string_literals;

    // clang-format off
    cl::set_program("tanto");
    cl::set_name("Tanto");
    cl::set_description("The Universal GUI");
    cl::set_version("1.0");

    cl::Options{
        cl::opt("d", "debug", "Debug mode"),
        cl::opt("b", "backend"_arg, "Select backend"),
    };

    cl::Usage{
        cl::cmd("stdin", *--"debug"__, *--"backend"__),
        cl::cmd("load", "filename", *--"debug"__, *--"backend"__),
        cl::cmd("message", "title", "text", *cl::one("info", "question", "warning", "error"), *--"debug"__, *--"backend"__),
        cl::cmd("confirm", "title", "text", *cl::one("info", "question", "warning", "error"), *--"debug"__, *--"backend"__),
        cl::cmd("input", "title", *"text"__, *"value"__, *--"debug"__, *--"backend"__),
        cl::cmd("password", "title", *"text"__, *--"debug"__, *--"backend"__),
        cl::cmd("selectdir", *"title"__, *"dir"__, *--"debug"__, *--"backend"__),
        cl::cmd("loadfile", *"title"__, *"filter"__, *"dir"__, *--"debug"__, *--"backend"__),
        cl::cmd("savefile", *"title"__, *"filter"__, *"dir"__, *--"debug"__, *--"backend"__),
        cl::cmd("list", *--"debug"__),
    };
    // clang-format on

    if(BACKENDS.empty()) {
        fmt::println("ERROR: No backends available");
        return 2;
    }

    selectedbackend = BACKENDS.begin()->first;

    auto args = cl::parse(argc, argv);

    if(args["debug"].to_bool()) {
        for(const auto& arg : args)
            fmt::println("{} - {}", arg.first, arg.second.dump());
    }

    if(args["list"].to_bool()) {
        for(const auto& [name, version] : BACKENDS)
            fmt::println("{}: {}", name, version);

        return 0;
    }

    if(!args["backend"]) {
        char* envbackend = std::getenv("TANTO_BACKEND");
        if(envbackend)
            selectedbackend = envbackend;
    }
    else
        selectedbackend = args["backend"].to_string();

    if(!has_backend(selectedbackend)) {
        fmt::println("ERROR: Unsupported backend '{}'", selectedbackend);
        return 1;
    }

    BackendPtr backend = new_backend(selectedbackend, argc, argv);

    if(needs_json(args))
        return execute_json(backend, args);
    return execute_mode(backend, args);
}
