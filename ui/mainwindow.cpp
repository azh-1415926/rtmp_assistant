#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QString>
#include <QProcess>

#include "jsonFile.hpp"

bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if (!targetDir.exists())
    {
        if (!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }
    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach (QFileInfo fileInfo, fileInfoList)
    {
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;
        if (fileInfo.isDir())
        {
            if (!copyDirectoryFiles(fileInfo.filePath(),
                                    targetDir.filePath(fileInfo.fileName()),
                                    coverFileIfExist))
                return false;
        }
        else
        {
            if (coverFileIfExist && targetDir.exists(fileInfo.fileName()))
            {
                targetDir.remove(fileInfo.fileName());
            }
            if (!QFile::copy(fileInfo.filePath(),
                             targetDir.filePath(fileInfo.fileName())))
            {
                return false;
            }
        }
    }
    return true;
}

QString getOBSPath()
{
    // 尝试从64位注册表读取（64位系统上的OBS安装）
    QSettings reg64("HKEY_LOCAL_MACHINE\\SOFTWARE\\OBS Studio", QSettings::NativeFormat);
    QString path64 = reg64.value(".").toString();

    if (!path64.isEmpty())
    {
        return path64;
    }

    // 尝试从32位注册表读取（32位系统或32位OBS安装）
    QSettings reg32("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\OBS Studio", QSettings::NativeFormat);
    QString path32 = reg32.value("InstallLocation").toString();

    if (!path32.isEmpty())
    {
        return path32;
    }

    // 如果注册表中没有找到，尝试常用安装路径
    QStringList commonPaths = {
        "C:\\Program Files\\obs-studio",
        "C:\\Program Files (x86)\\obs-studio",
        "D:\\Program Files\\obs-studio",
        "D:\\Program Files (x86)\\obs-studio",
        "E:\\Program Files\\obs-studio",
        "E:\\Program Files (x86)\\obs-studio"
    };

    for (const QString &path : commonPaths)
    {
        if (QDir(path).exists())
        {
            return path;
        }
    }

    return QString(); // 返回空字符串表示未找到
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), _rtmp(new rtmp()), rtmp_thread(nullptr), flagOfStop(0)
{
    ui->setupUi(this);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete _rtmp;

    // 释放线程
    if (rtmp_thread != nullptr)
    {
        rtmp_thread->quit();
        rtmp_thread->wait();
        delete rtmp_thread;
    }
}

void MainWindow::init()
{
    QStringList listOfTime;
    for (int i = 0; i < 20; i++)
    {
        listOfTime << QString::number((i + 1) * 5);
    }
    // 添加监听时间
    ui->timeOfRTMP->addItems(listOfTime);
    ui->timeOfRTMP->setCurrentIndex(1);

    // 获取所有网卡
    auto interface_list = _rtmp->interfaces_name;

    for (auto i : interface_list)
    {
        interface_names.push_back(QString::fromStdString(i.first));
        interface_descriptions.push_back(QString::fromStdString(i.second));
    }

    // 用户选择网卡，则将当前网卡信息传入 get_rtmp 子线程
    connect(ui->optionsOfInterface, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int i)
            {
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
        }); });

    // 将所有网卡添加到下拉框
    ui->optionsOfInterface->addItems(interface_descriptions);

    // 获取按钮所绑定的函数
    connect(ui->btnOfRTMP, &QPushButton::clicked, this, [=]()
            {
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
        } });

    QString obs_config_root = QDir::homePath() + "/AppData/Roaming/obs-studio/basic/profiles";
    qDebug() << "Obs Config Path:" << obs_config_root;

    QString settingDir = obs_config_root + "/rtmp_assistant";

    // copy obs_config_root+'/默认' directory to obs_config_root+'/rtmp_assistant'
    copyDirectoryFiles(obs_config_root + "/默认", settingDir, false);

    ui->pathOfSettingDir->setText(settingDir);

    connect(ui->btnOfExport, &QPushButton::clicked, this, [=]()
            {
                QString jsonPath = obs_config_root + "/rtmp_assistant/service.json";
                jsonFile jsonfile(jsonPath);
                if (!jsonfile.isLoad())
                {
                    qDebug() << "Read obs settings Failed!";
                    ui->outputOfExport->setText("无法打开配置文件");
                    return;
                }

                if (ui->server->text().isEmpty() && ui->code->toPlainText().isEmpty())
                {
                    ui->outputOfExport->setText("请先获取推流码");
                    return;
                }

                QJsonObject setting = jsonfile.value("settings").toObject();
                setting.insert("server", ui->server->text());
                setting.insert("key", ui->code->toPlainText());

                jsonfile.add("settings", setting);
                jsonfile.save(jsonPath);

                ui->outputOfExport->setText("导出成功");
                
                QString obsPath=getOBSPath();

                if(!obsPath.isEmpty())
                {
                    qDebug()<<"Get Obs Path : "<<obsPath;

                    QString obs64bitPath=obsPath+"\\bin\\64bit\\obs64.exe";
                    QFile obs64bit(obs64bitPath);
                    if(obs64bit.exists())
                    {
                        QProcess::startDetached(obs64bitPath);
                        return;
                    }

                    QString obs32bitPath=obsPath+"\\bin\\32bit\\obs32.exe";
                    QFile obs32bit(obs32bitPath);
                    if(obs32bit.exists())
                    {
                        QProcess::startDetached(obs32bitPath);
                        return;
                    }
                }
            });
}
