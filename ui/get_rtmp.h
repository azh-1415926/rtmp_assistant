#pragma once

#include <QThread>

#include "rtmp.h"

class get_rtmp : public QThread
{
    Q_OBJECT
public:
    std::string interface_name;
    get_rtmp(int& f,int time,QObject *parent=nullptr);
    get_rtmp(const get_rtmp& r);
    ~get_rtmp();

    rtmp* _rtmp;
    int& flag;
    int timeout;

    void run() override;

signals:
    void sendServerInfo(const QString&,const QString&);

public slots:
};