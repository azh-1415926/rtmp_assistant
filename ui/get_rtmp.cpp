#include "get_rtmp.h"
#include <QDebug>
#include <QThread>

get_rtmp::get_rtmp(int& f,int time,QObject *parent) : QThread(parent), _rtmp(new rtmp), flag(f), timeout(time)
{
    azh::logger()<<"create get_rtmp(f,time).";
}

get_rtmp::get_rtmp(const get_rtmp &r): _rtmp(new rtmp), flag(r.flag), timeout(r.timeout) 
{
    azh::logger()<<"copy a get_rtmp.";
}

get_rtmp::~get_rtmp()
{
    delete _rtmp;
    azh::logger()<<"destory get_rtmp.";
}

void get_rtmp::run()
{
    azh::logger()<<"Start get_rtmp thread.";
    azh::logger()<<"get_rtmp by flag:"<<flag<<",timeout:"<<timeout;

    _rtmp->getByInterface(interface_name,flag,timeout);

    azh::logger()<<"get_rtmp finish.";

    emit sendServerInfo(QString::fromStdString(_rtmp->getServer()),QString::fromStdString(_rtmp->getCode()));

    azh::logger()<<"The get_rtmp thread end.";
}

