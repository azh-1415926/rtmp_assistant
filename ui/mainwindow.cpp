#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QLineEdit>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _rtmp(new rtmp())
{
    ui->setupUi(this);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete _rtmp;
}

void MainWindow::init()
{
    // ui->server->setEnabled(false);
    // ui->code->setEnabled(false);

    auto interface_list=_rtmp->interfaces_name;

    for(auto i:interface_list)
    {
        interface_names.push_back(QString::fromStdString(i.first));
        interface_descriptions.push_back(QString::fromStdString(i.second));
    }

    ui->optionsOfInterface->addItems(interface_descriptions);

    connect(ui->optionsOfInterface,&QComboBox::currentIndexChanged,this,[=](int i){
        if(this->_rtmp->getByInterface(i<interface_names.size()?interface_names.at(i).toStdString():""))
        {
            ui->server->setText(QString::fromStdString(this->_rtmp->getServer()));
            ui->code->setText(QString::fromStdString(this->_rtmp->getCode()));
        }
        else
        {
            ui->server->setText("获取失败");
            ui->code->setText("获取失败");
        }
    });
}
