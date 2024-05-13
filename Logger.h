#pragma once

#include <string>

#include "noncopyable.h"

// ʹ�÷�����LOG_INFO("%s %d", arg1,arg2) �ú������տɱ��
// �ڴ�����Ŀ�У�Ϊ�˷�ֹ�����ü��д������һ�㶼����do{}while(0),��ÿһ��ĩβ��Ҫ��\�Ҳ��ܼӿո�
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

// ��ӡ����־ֱ���˳�
#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0)

// ��������˺�MUDEBUG�������DEBUG���������
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// ������־�ļ��� 
// INFO����ӡ��Ҫ������Ϣ
// ERROR����Ӱ�������������ִ�еĴ�����Ϣ
// FATAL: �����µĴ����ϵͳ�޷������������У�����ؼ���Ϣ��exit
// DEBUG: ������Ϣ��ϵͳ��������ʱĬ�ϰ�DEBUG�ص�����Ҫʱ���ã�����ͨ�����û��ߺ�ʹ��

enum LogLevel
{
    INFO,  // ��ͨ��Ϣ
    ERROR, // ������Ϣ
    FATAL, // core��Ϣ
    DEBUG, // ������Ϣ
};

//���һ����־��
class Logger : noncopyable
{
public:
    // ��ȡ��־Ψһ��ʵ������
    static Logger& instance();
    // ������־����
    void setLogLevel(int Level);
    // д��־
    void log(std::string msg);
private:
    int logLevel_;
    // // ˽�еĹ��캯��
    // Logger(){}
};