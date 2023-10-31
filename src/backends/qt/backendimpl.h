#pragma once

#include <QApplication>
#include <QObject>
#include "../../backend.h"

class MainWindow;

class BackendQtImpl : public Backend {
public:
    BackendQtImpl(int& argc, char** argv);
    ~BackendQtImpl() override;
    int run() const override;
    void exit() override;
    nlohmann::json get_model_data(const tanto::types::Widget& arg, const std::any& w) override;
    void message(const std::string& title, const std::string& text, MessageType mt, MessageIcon icon) override;
    void input(const std::string& title, const std::string& text, const std::string& value, InputType it) override;
    void select_dir(const std::string& title, const std::string& startdir) override;
    void load_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir) override;
    void save_file(const std::string& title, const tanto::FilterList& filter, const std::string& startdir) override;
    static std::string_view version();

private:
    std::any new_window(const tanto::types::Window& arg) override;
    std::any new_space(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_text(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_input(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_number(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_image(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_button(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_check(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_list(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_tree(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_tabs(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_row(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_column(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_grid(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_form(const tanto::types::Widget& arg, const std::any& parent) override;
    std::any new_group(const tanto::types::Widget& arg, const std::any& parent) override;

private:
    QApplication m_app;
    MainWindow* m_mainwindow{nullptr};
};
