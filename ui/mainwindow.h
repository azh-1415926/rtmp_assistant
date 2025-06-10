#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "rtmp.h"
#include "get_rtmp.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    rtmp* _rtmp;
    QList<QString> interface_names;
    QList<QString> interface_descriptions;
    int flagOfStop;

    get_rtmp* rtmp_thread;

    void init();
};
#endif // MAINWINDOW_H
