#pragma once

#include <string>
#include <any>
#include "types.h"
#include "tanto.h"
#include "events.h"

class Backend : public Events {
public:
    enum class MessageType {
        MESSAGE = 0,
        CONFIRM
    };

    enum class MessageIcon {
        NONE = 0,
        INFO,
        WARNING,
        QUESTION,
        ERROR
    };

    enum class InputType {
        NORMAL = 0,
        PASSWORD
    };

public:
    Backend(int& argc, char** argv);
    virtual ~Backend() = default;
    virtual int run() const = 0;
    void process(const tanto::types::Window& arg);
    virtual void message(const std::string& title, const std::string& text, MessageType mt, MessageIcon icon) = 0;
    virtual void input(const std::string& title, const std::string& text, const std::string& value, InputType it) = 0;
    virtual void load_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir) = 0;
    virtual void save_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir) = 0;
    virtual void select_dir(const std::string& title, const std::string& startdir) = 0;
    static std::string_view version();

private:
    virtual std::any new_window(const tanto::types::Window& arg) = 0;
    virtual std::any new_space(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_text(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_input(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_number(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_image(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_button(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_check(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_list(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_tree(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_tabs(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_row(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_column(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_grid(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_form(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual std::any new_group(const tanto::types::Widget& arg, const std::any& parent) = 0;
    virtual void widget_processed(const tanto::types::Widget& arg, const std::any& widget);
    virtual void processed();
    std::any process_container(const std::any& layout, const tanto::types::Widget& arg);
    std::any process(const tanto::types::Widget& req, const std::any& parent);

    template <typename Function>
    std::any process_layout(const tanto::types::Widget& arg, const std::any& parent, Function f) {
        if(arg.has_group()) {
            tanto::types::Widget w{"group"};
            w.text = arg.group;
            return this->process_container(f(this->new_group(w, parent)), arg);
        }

        return this->process_container(f(parent), arg);
    }
};
