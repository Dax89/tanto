#include "src/backend.h"
#include "src/error.h"
#include "src/tanto.h"
#include <cl/cl.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

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

const std::unordered_map<std::string_view, std::string_view> BACKENDS {
#if defined(BACKEND_QT)
    {"qt", BackendQtImpl::version()},
#endif
#if defined(BACKEND_GTK)
        {"gtk", BackendGtkImpl::version()},
#endif
};

std::string selected_backend{"gtk"};

using BackendPtr = std::unique_ptr<Backend>;

[[nodiscard]] BackendPtr new_backend(const std::string& name, int argc,
                                     char** argv) {

#if defined(BACKEND_QT)
    if(name == "qt")
        return std::make_unique<BackendQtImpl>(argc, argv);
#endif // defined(BACKEND_QT)

#if defined(BACKEND_GTK)
    if(name == "gtk")
        return std::make_unique<BackendGtkImpl>(argc, argv);
#endif // defined(BACKEND_GTK)

    except("Backend '{}' not found", name);
}

tanto::FilterList parse_filter(const cl::Value& v) {
    return tanto::parse_filter(v ? v.to_stringview() : std::string_view{});
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

int execute_mode(const BackendPtr& backend, cl::Values& values) {
    if(values["list"].to_bool()) {
        for(const auto& [name, version] : BACKENDS)
            fmt::println("{}: {}", name, version);
    }
    else if(values["message"].to_bool() || values["confirm"].to_bool()) {
        Backend::MessageIcon icon = Backend::MessageIcon::NONE;

        if(values["info"].to_bool())
            icon = Backend::MessageIcon::INFO;
        else if(values["question"].to_bool())
            icon = Backend::MessageIcon::QUESTION;
        else if(values["warning"].to_bool())
            icon = Backend::MessageIcon::WARNING;
        else if(values["error"].to_bool())
            icon = Backend::MessageIcon::ERROR;

        backend->message(
            values["title"].to_string(), values["text"].to_string(),
            values["confirm"].to_bool() ? Backend::MessageType::CONFIRM
                                        : Backend::MessageType::MESSAGE,
            icon);
    }
    else if(values["input"].to_bool() || values["password"].to_bool()) {
        std::string text =
            values["text"] ? values["text"].to_string() : std::string{};
        std::string value =
            values["value"] ? values["value"].to_string() : std::string{};
        backend->input(values["title"].to_string(), text, value,
                       values["password"].to_bool()
                           ? Backend::InputType::PASSWORD
                           : Backend::InputType::NORMAL);
    }
    else if(values["selectdir"].to_bool()) {
        backend->select_dir(
            values["title>"] ? values["title"].to_string() : std::string{},
            values["dir>"] ? values["dir"].to_string() : std::string{});
    }
    else if(values["loadfile"].to_bool()) {
        backend->load_file(
            values["title>"] ? values["title"].to_string() : std::string{},
            parse_filter(values["filter"]),
            values["dir>"] ? values["dir"].to_string() : std::string{});
    }
    else if(values["savefile"].to_bool()) {
        backend->save_file(
            values["title"] ? values["title"].to_string() : std::string{},
            parse_filter(values["filter"]),
            values["dir"] ? values["dir"].to_string() : std::string{});
    }
    else
        unreachable;

    return 0;
}

bool needs_json(cl::Values& values) {
    return values["stdin"].to_bool() || values["load"].to_bool();
}

int execute_json(const BackendPtr& backend, cl::Values& values) {
    nlohmann::json jsonreq;

    try {
        if(values["stdin"].to_bool())
            jsonreq = nlohmann::json::parse(read_stdin());
        else if(values["load"].to_bool()) {
            std::ifstream f{std::string{values["filename"].to_string()}};
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

    auto options = cl::parse(argc, argv);

    if(options["debug"].to_bool()) {
        for(const auto& arg : options)
            fmt::println("{} - {}", arg.first, arg.second.dump());
    }

    if(!options["backend"]) {
        char* envbackend = std::getenv("TANTO_BACKEND");
        if(envbackend)
            selected_backend = envbackend;
    }
    else
        selected_backend = options["backend"].to_string();

    if(!BACKENDS.count(selected_backend)) {
        fmt::println("ERROR: Unsupported backend '{}'", selected_backend);
        return 1;
    }

    BackendPtr backend = new_backend(selected_backend, argc, argv);

    if(needs_json(options))
        return execute_json(backend, options);
    return execute_mode(backend, options);
}
