#include "backend.h"
#include "error.h"
#include "utils.h"
#include <unordered_map>

namespace {

std::unordered_map<std::string, std::any> m_model;

} // namespace

Backend::Backend(int& argc, char* argv[]): Events{} { (void)argc; (void)argv; }
void Backend::processed() { }
void Backend::widget_processed(const tanto::types::Widget& arg, const std::any& widget) { (void)arg; (void)widget; }
std::string_view Backend::version() { unreachable; }

void Backend::process(const tanto::types::Window& arg)
{
    std::any window = this->new_window(arg);
    if(arg.body) this->process(arg.body, window);
    this->processed();
}

std::any Backend::process(const tanto::types::Widget& arg, const std::any& parent)
{
    using namespace tanto::utils::string_literals;

    if(!arg.title.empty()) // Wrap widget vertically
    {
        tanto::types::Widget t{"text"};
        t.text = arg.title;

        tanto::types::Widget w = arg; // Copy & Clear title
        w.fill = true;
        w.title.clear();

        tanto::types::Widget l{"column"};
        l.fill = arg.fill;
        l.items.push_back(t);
        l.items.push_back(w);

        return this->process(l, parent);
    }

    std::any widget;

    switch(tanto::utils::fnv1a_32(arg.type))
    {
        case "space"_fnv1a_32:  widget = this->new_space(arg, parent); break;
        case "text"_fnv1a_32:   widget = this->new_text(arg, parent); break;
        case "input"_fnv1a_32:  widget = this->new_input(arg, parent); break;
        case "number"_fnv1a_32: widget = this->new_number(arg, parent); break;
        case "image"_fnv1a_32:  widget = this->new_image(arg, parent); break;
        case "button"_fnv1a_32: widget = this->new_button(arg, parent); break;
        case "check"_fnv1a_32:  widget = this->new_check(arg, parent); break;
        case "list"_fnv1a_32:   widget = this->new_list(arg, parent); break;
        case "tree"_fnv1a_32:   widget = this->new_tree(arg, parent); break;
        case "tabs"_fnv1a_32:   widget = this->process_container(this->new_tabs(arg, parent), arg); break;
        case "row"_fnv1a_32:    widget = this->process_layout(arg, parent, [&](auto&& p) { return this->new_row(arg, p); }); break;
        case "column"_fnv1a_32: widget = this->process_layout(arg, parent, [&](auto&& p) { return this->new_column(arg, p);}); break;
        case "grid"_fnv1a_32:   widget = this->process_layout(arg, parent, [&](auto&& p) { return this->new_grid(arg, p); }); break;
        case "form"_fnv1a_32:   widget = this->process_layout(arg, parent, [&](auto&& p) { return this->new_form(arg, p); }); break;

        default:
            if(arg.type.empty()) return nullptr;
            except("Unknown widget type: '{}'", arg.type);
            break;
    }

    assume(widget.has_value());
    if(arg.has_id()) m_model[arg.id] = widget;

    this->widget_processed(arg, widget);
    return widget;
}

std::any Backend::process_container(const std::any& container, const tanto::types::Widget& arg)
{
    for(auto item : arg.items)
    {
        std::visit(tanto::utils::Overload{
            [&](tanto::types::Widget& a) { this->process(a, container); },
            [&](std::string& a) { // Convert strings in 'text' widgets
                tanto::types::Widget w{"text"};
                w.text = a;
                this->process(w, container);
            },
            [](auto&) { } // Ignore everything else
        }, item);
    }

    return container;
}
