#include "mainwindow.h"
#include <QApplication>
#include <QEvent>

MainWindow::MainWindow(QWidget* parent): QMainWindow{parent} { }

bool MainWindow::event(QEvent* event)
{
    if(event->type() == QEvent::Close && this->windowFlags() & Qt::Popup)
        qApp->exit();

    return QMainWindow::event(event);
}
