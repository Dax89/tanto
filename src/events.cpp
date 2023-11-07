#include "events.h"
#include "error.h"
#include "utils.h"
#include <cstdio>
#include <variant>

Events::ProcessedModel Events::process_model() {
    assume(m_ismodel);

    ProcessedModel pmodel;

    for(const auto& [id, arg] : m_model) {
        nlohmann::json data = this->get_model_data(arg.first, arg.second);
        if(!data.is_null())
            pmodel[id] = data;
    }

    return pmodel;
}

void Events::selected(const tanto::types::Widget& w,
                      const nlohmann::json& row) {
    if(m_ismodel)
        return;

    this->create_event("selected", w, row);
    this->exit();
}

void Events::selected(const tanto::types::Widget& w, int index,
                      tanto::types::MultiValue value) {
    if(m_ismodel)
        return;

    nlohmann::json detail = {{"index", index}};

    std::visit(tanto::utils::Overload{
                   [&](tanto::types::Widget& a) { detail["id"] = a.get_id(); },
                   [&](std::string& a) { detail["id"] = a; }},
               value);

    this->create_event("selected", w, detail);
}

void Events::changed(const tanto::types::Widget& w,
                     const nlohmann::json& detail) {
    if(m_ismodel)
        return;

    this->create_event("changed", w, detail);
}

void Events::clicked(const tanto::types::Widget& w,
                     const nlohmann::json& detail) {
    this->create_event("clicked", w, detail);
    this->exit();
}

void Events::double_clicked(const tanto::types::Widget& w,
                            const nlohmann::json& detail) {
    if(m_ismodel)
        return;

    this->create_event("doubleclicked", w, detail);
    this->exit();
}

void Events::send_event(const std::string& s) { std::puts(s.c_str()); }

void Events::create_event(const std::string& type,
                          const tanto::types::Widget& w,
                          const nlohmann::json& detail) {
    assume(!w.id.empty());

    nlohmann::json event = {{"type", type}, {"from", w.id}};

    if(m_ismodel)
        event["detail"] = this->process_model();
    else if(!detail.is_null())
        event["detail"] = detail;

    std::puts(event.dump().c_str());
}
