#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _rtmp(new rtmp())
    , rtmp_thread(nullptr)
    , flagOfStop(0)
{
    ui->setupUi(this);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete _rtmp;

    // 释放线程
    if(rtmp_thread!=nullptr)
    {
        rtmp_thread->quit();
        rtmp_thread->wait();
        delete rtmp_thread;
    } 
}

void MainWindow::init()
{
    QStringList listOfTime;
    for(int i=0;i<20;i++)
    {
        listOfTime<<QString::number((i+1)*5);
    }
    // 添加监听时间
    ui->timeOfRTMP->addItems(listOfTime);
    ui->timeOfRTMP->setCurrentIndex(1);

    // 获取所有网卡
    auto interface_list=_rtmp->interfaces_name;

    for(auto i:interface_list)
    {
        interface_names.push_back(QString::fromStdString(i.first));
        interface_descriptions.push_back(QString::fromStdString(i.second));
    }

    // 用户选择网卡，则将当前网卡信息传入 get_rtmp 子线程
    connect(ui->optionsOfInterface,&QComboBox::currentIndexChanged,this,[=](int i){
        std::string interface_name=i<interface_names.size()?interface_names.at(i).toStdString():"";

        if(interface_name.empty())
        {
            return;
        }

        // 若重复运行，先释放子线程对象
        if(rtmp_thread!=nullptr)
        {
            rtmp_thread->quit();
            rtmp_thread->wait();
            delete rtmp_thread;
        }

        // 创建子线程
        rtmp_thread= new get_rtmp(flagOfStop,ui->timeOfRTMP->currentText().toInt());
        rtmp_thread->interface_name=interface_name;

        // 将子线程 sendServerInfo 信号绑定匿名函数
        connect(rtmp_thread,&get_rtmp::sendServerInfo,this,[=](const QString& server,const QString& code){
            ui->server->setText(server);
            ui->code->setText(code);

            if(server.isEmpty()&&code.isEmpty())
            {
                ui->outputOfRTMP->setText("获取失败，未发现任何 rtmp 网络请求，请点击开始获取，再打开直播软件");
            }
            else
            {
                ui->outputOfRTMP->setText("获取成功！");
            }

            ui->btnOfRTMP->setText("开始获取");
            this->flagOfStop=0;
        });
    });

    // 将所有网卡添加到下拉框
    ui->optionsOfInterface->addItems(interface_descriptions);

    // 获取按钮所绑定的函数
    connect(ui->btnOfRTMP,&QPushButton::clicked,this,[=](){
        if(ui->btnOfRTMP->text()=="开始获取")
        {
            if(!rtmp_thread)
            {
                return;
            }

            rtmp_thread->setTimeout(ui->timeOfRTMP->currentText().toInt());
            rtmp_thread->start();

            ui->btnOfRTMP->setText("停止获取");
            ui->outputOfRTMP->setText("正在获取推流码...");
        }
        else if(ui->btnOfRTMP->text()=="停止获取")
        {
            if(!rtmp_thread)
            {
                return;
            }

            this->flagOfStop=1;
            ui->btnOfRTMP->setText("开始获取");

            QThread::sleep(1);

            this->flagOfStop=0;
        }
    });
}
