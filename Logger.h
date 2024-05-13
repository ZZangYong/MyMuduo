#pragma once

#include <string>

#include "noncopyable.h"

// 使用方法：LOG_INFO("%s %d", arg1,arg2) 用宏来接收可变参
// 在大型项目中，为了防止宏代表好几行代码出错，一般都是用do{}while(0),且每一行末尾都要加\且不能加空格
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

// 打印完日志直接退出
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

// 如果定义了宏MUDEBUG，才输出DEBUG，否则不输出
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

// 定义日志的级别 
// INFO：打印重要流程信息
// ERROR：不影响软件继续向下执行的错误信息
// FATAL: 毁灭新的打击，系统无法继续向下运行，输出关键信息后exit
// DEBUG: 调试信息，系统正常运行时默认把DEBUG关掉，需要时再用，比如通过配置或者宏使用

enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

//输出一个日志类
class Logger : noncopyable
{
public:
    // 获取日志唯一的实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int Level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_;
    // // 私有的构造函数
    // Logger(){}
};