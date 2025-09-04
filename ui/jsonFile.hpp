#ifndef SETTING_FILE_H
#define SETTING_FILE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

#include "logger.hpp"

/* 用于读取导出 json 配置文件 */
class jsonFile : public QObject
{
    Q_OBJECT

    private:
        /* Json 对象、Json 文档 */
        QJsonObject m_Json;
        QJsonDocument m_Doc;
        /* 读取的标志 */
        int m_LoadFlag;

    public:
        explicit jsonFile(QObject* parent = nullptr): QObject(parent), m_LoadFlag(0){ };
        explicit jsonFile(const QString& filepath,QObject* parent = nullptr)
            : QObject(parent), m_LoadFlag(0){ load(filepath); };
        explicit jsonFile(const QJsonObject& json,QObject* parent = nullptr)
            : QObject(parent), m_Json(json), m_LoadFlag(0){ };
        virtual ~jsonFile(){ };

        template<class T>
        void add(const QString& key,T value)
        {
            m_Json.insert(key,value);
        }

        template<class T>
        void add(const QString& key,QList<T> value)
        {
            QJsonArray array;
            for(int i=0;i<value.length();i++)
                array.push_back(value[i]);
            m_Json.insert(key,array);
        }

        template<class T1,class T2=T1>
        void add(const QString& key,QList<QPair<T1,T2>> value)
        {
            QJsonObject obj;

            for(int i=0;i<value.length();i++)
            {
                obj.insert(value[i].first,value[i].second);
            }

            m_Json.insert(key,obj);
        }

        template<class T1,class T2=T1>
        void addArray(const QString& key,QList<QPair<T1,T2>> value)
        {
            QJsonArray array;
            QJsonArray first;
            QJsonArray second;
            for(int i=0;i<value.length();i++)
            {
                first.push_back(value[i].first);
                second.push_back(value[i].second);
            }
            array.push_back(first);
            array.push_back(second);
            m_Json.insert(key,array);
        }

        /* 删除指定关键字 */
        inline void remove(const QString& key){ m_Json.remove(key); }
        /* 判定是否已导入 */
        inline bool isLoad() const { return m_LoadFlag==1; }
        /* 获取所有关键字 */
        inline QStringList keys(){ return m_Json.keys(); }
        /* 获取配置中的的对应属性值 */
        inline QJsonValue value(const QString& key) const { return m_Json.value(key); }
        /* 清空 */
        inline void clear()
        {
            QStringList keys=m_Json.keys();
            for(const auto& key : keys)
                m_Json.remove(key);
        }

        QString toString()
        {
            QString str;
            /* stream 处理文件，设置编码为 utf-8 */
            QTextStream stream(&str);
            #if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
                stream.setCodec("utf-8");
            #elif (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
                stream.setEncoding(QStringConverter::Utf8);
            #endif
            /* 把 json 对象转成 json 文档格式 */
            m_Doc.setObject(m_Json);
            /* 导出 json、并关闭文件 */
            stream<<m_Doc.toJson();

            return str;
        }

    protected:
        ;

    public slots:
        /* 导入 json 对象 */
        void load(const QJsonObject& json)
        {
            m_Json=json;
            if(m_LoadFlag)
                emit reloaded();
            else
                m_LoadFlag=1;
            emit loaded();
        }
        /* 导入 json 文件 */
        void load(const QString& path)
        {
            QFile file(path);
            if(!file.open(QIODevice::ReadOnly|QFile::Text))
                return;
            /*
                error 用于接收解析错误
                stream 用于读取文件
                str 存储字符串格式的 json 文件
            */
            QJsonParseError error;
            QTextStream stream(&file);
            QString str=stream.readAll();
            file.close();
            /* 将 json 字符串解析为 QJson 文件对象，并存储错误信息 */
            m_Doc=QJsonDocument::fromJson(str.toUtf8(),&error);
            if(error.error!=QJsonParseError::NoError&&!m_Doc.isNull())
            {
                azh::logger()<<"json parse error!";
                return;
            }
            /* json 接收 QJson 文件对象里的 QJson 根节点 */
            m_Json=m_Doc.object();
            if(m_LoadFlag)
                emit reloaded();
            else
                m_LoadFlag=1;
            emit loaded();
        }
        /* 获得内部 json 对象 */
        inline QJsonObject toJson(){ return m_Json; }
        /* 通过 json 文档 导出为 json 对象 */
        static QJsonObject toJson(const QString& str)
        {
            QJsonParseError error;
            QJsonDocument doc=QJsonDocument::fromJson(str.toUtf8(),&error);
            if(error.error!=QJsonParseError::NoError&&!doc.isNull())
            {
                azh::logger()<<"json parse error!";
                return QJsonObject();
            }
            return doc.object();
        }
        void fromJson(const QString& str) { m_Json=toJson(str); }
        /* 导出为 json 文件 */
        void save(const QString& path)
        {
            if(m_Json.isEmpty())
                return;
            QFile file(path);
            /* 保存到 path 文件中（只写、截断保存） */
            if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))
            {
                /* stream 处理文件，设置编码为 utf-8 */
                QTextStream stream(&file);
                #if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
                    stream.setCodec("utf-8");
                #elif (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
                    stream.setEncoding(QStringConverter::Utf8);
                #endif
                /* 把 json 对象转成 json 文档格式 */
                m_Doc.setObject(m_Json);
                /* 导出 json、并关闭文件 */
                stream<<m_Doc.toJson();
                file.close();
                emit saved();
            }
            else
                azh::logger()<<"Save json to \""+path.toStdString()+"\" failed!";
        }

    signals:
        void loaded();
        void reloaded();
        void saved();

    private:
        ;
};

#endif