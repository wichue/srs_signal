#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "srs_app_server.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    SrsSignalManager* pSrsSignalManager;
};
#endif // MAINWINDOW_H
