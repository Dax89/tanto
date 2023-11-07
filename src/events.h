#pragma once

#include "types.h"
#include <any>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

class Events {
private:
    using ProcessedModel = std::unordered_map<std::string, nlohmann::json>;
    using Model = std::unordered_map<std::string,
                                     std::pair<tanto::types::Widget, std::any>>;

public:
    virtual void exit() = 0;
    virtual nlohmann::json get_model_data(const tanto::types::Widget& arg,
                                          const std::any& w) = 0;
    void selected(const tanto::types::Widget& w, int index,
                  tanto::types::MultiValue value);
    void selected(const tanto::types::Widget& w,
                  const nlohmann::json& row = {});
    void changed(const tanto::types::Widget& w,
                 const nlohmann::json& detail = {});
    void clicked(const tanto::types::Widget& w,
                 const nlohmann::json& detail = {});
    void double_clicked(const tanto::types::Widget& w,
                        const nlohmann::json& detail = {});
    void send_event(const std::string& s);

    inline void send_quit_event(const std::string& s) {
        this->send_event(s);
        this->exit();
    }

private:
    void create_event(const std::string& type, const tanto::types::Widget& w,
                      const nlohmann::json& detail);
    ProcessedModel process_model();

protected:
    bool m_ismodel{false};
    Model m_model;
};
