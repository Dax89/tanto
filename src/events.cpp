#include "events.h"
#include <cstdio>
#include <variant>
#include "utils.h"
#include "error.h"

namespace {

void create_event(const std::string& type, const tanto::types::Widget& w, const nlohmann::json& detail)
{
    assume(!w.id.empty());

    nlohmann::json event = {
        {"type", type},
        {"from", w.id}
    };

    if(!detail.is_null()) event["detail"] = detail;
    std::puts(event.dump().c_str());
}

} // namespace

void Events::selected(const tanto::types::Widget& w, int index, tanto::types::MultiValue value, const nlohmann::json& detail)
{
    nlohmann::json fulldetail = {
        {"index", index}
    };

    if(detail.is_object())
    {
        for(const auto& [k, v] : detail.items())
            fulldetail[k] = v;
    }

    std::visit(tanto::utils::Overload{
            [&](tanto::types::Widget& a) { fulldetail["id"] = a.get_id(); },
            [&](std::string& a) { fulldetail["id"] = a; }
    }, value);

    create_event("selected", w, fulldetail);
    this->exit();
}

void Events::changed(const tanto::types::Widget& w, const nlohmann::json& detail)
{
    create_event("changed", w, detail);
    this->exit();
}

void Events::clicked(const tanto::types::Widget& w, const nlohmann::json& detail)
{ 
    create_event("clicked", w, detail);
    this->exit();
}

void Events::double_clicked(const tanto::types::Widget& w, const nlohmann::json& detail)
{ 
    create_event("doubleclicked", w, detail);
    this->exit();
}

void Events::send_event(const std::string& s) { std::puts(s.c_str()); }
