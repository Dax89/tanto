#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "types.h"

class Events
{
public:
    virtual void exit() = 0;
    void selected(const tanto::types::Widget& w, int index, tanto::types::MultiValue value, const nlohmann::json& detail = {}, const nlohmann::json& row = {});
    void changed(const tanto::types::Widget& w, const nlohmann::json& detail = {});
    void clicked(const tanto::types::Widget& w, const nlohmann::json& detail = {});
    void double_clicked(const tanto::types::Widget& w, const nlohmann::json& detail = {});
    void send_event(const std::string& s);

    inline void send_quit_event(const std::string& s) {
        this->send_event(s);
        this->exit();
    }
};
