#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <ctime>
#include <mutex>

#ifndef LOGGER_STREAM

#define LOGGER_STREAM std::cout

#endif

#define LOGGER_WARNNING 0
#define LOGGER_INFO 1
#define LOGGER_ERROR 2
#define LOGGER_FATAL 3

namespace azh
{
    enum class LOGGER_LEVEL
    {
        WARNNIING=LOGGER_WARNNING,INFO=LOGGER_INFO,ERROR_LEVEL=LOGGER_ERROR,FATAL=LOGGER_FATAL
    };

    class GlobalLoggerLevel
    {
        public:
            static GlobalLoggerLevel& getInstance()
            {
                static GlobalLoggerLevel instance;
                return instance;
            }
            LOGGER_LEVEL get() {  return m_Instance; }
            void set(const LOGGER_LEVEL& data) { m_Instance=data; }
        private:
            explicit GlobalLoggerLevel() { }
            LOGGER_LEVEL m_Instance;
    };

    static std::mutex Logger_Mutex;

    class logger
    {
        private:
            bool shouldLogging;

        public:
            template<class T>
            logger& operator<<(const T& type)
            {
                if(!shouldLogging)
                {
                    return *this;
                }

                LOGGER_STREAM<<type;

                return *this;
            }

            template<class T1,class T2>
            logger& operator<<(const T1& type)
            {
                if(!shouldLogging)
                {
                    return *this;
                }

                LOGGER_STREAM<<type.toStdString();

                return *this;
            }

            logger(int level=LOGGER_INFO)
            {
                if((LOGGER_LEVEL)level<GlobalLoggerLevel::getInstance().get())
                {
                    shouldLogging=false;
                    return;
                }

                Logger_Mutex.lock();
                shouldLogging=true;

                printCurrentTime();
                printLoggerHeader(level);
                
                LOGGER_STREAM<<" ";
            }

            ~logger()
            {
                if(!shouldLogging)
                {
                    return;
                }

                LOGGER_STREAM<<"\n";

                Logger_Mutex.unlock();
            }

            logger(const logger& l)=delete;
            logger operator=(const logger& l)=delete;
            
            void* operator new(size_t size)=delete;
            void* operator new[](size_t size)=delete;
            void operator delete(void* ptr)=delete;
            void operator delete[](void* ptr)=delete;

            static void setGlobalLevel(const LOGGER_LEVEL& level) { GlobalLoggerLevel::getInstance().set(level); }
            static void setGlobalLevel(int level) { GlobalLoggerLevel::getInstance().set((LOGGER_LEVEL)level); }

        private:
            void printLoggerHeader(const LOGGER_LEVEL& level) { printLoggerHeader(((int)level)); }
            void printLoggerHeader(int level)
            {
                LOGGER_STREAM<<"[";

                switch (level)
                {
                case LOGGER_WARNNING:
                    LOGGER_STREAM<<"WARNNING";
                    break;
                
                case LOGGER_INFO:
                    LOGGER_STREAM<<"  INFO  ";
                    break;

                case LOGGER_ERROR:
                    LOGGER_STREAM<<"  ERROR ";
                    break;

                case LOGGER_FATAL:
                    LOGGER_STREAM<<"  FATAL ";
                    break;

                default:
                    break;
                }
                LOGGER_STREAM<<"]";
            }

            void printCurrentTime()
            {
                time_t now;
                time(&now);
                tm p=*localtime(&now);

                LOGGER_STREAM<<"["<<
                    p.tm_year + 1900                        <<"-"<<
                    (p.tm_mon + 1)/10<<(p.tm_mon + 1)%10    <<"-"<<
                    p.tm_mday/10<<p.tm_mday%10              <<" "<<
                    p.tm_hour/10<<p.tm_hour%10              <<":"<<
                    p.tm_min/10<<p.tm_min%10                <<":"<<
                    p.tm_sec/10<<p.tm_sec%10                <<"]";
            }
    };
}

#endif