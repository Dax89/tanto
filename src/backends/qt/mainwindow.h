#pragma once

#include <QMainWindow>

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

protected:
    bool event(QEvent* event) override;
};
