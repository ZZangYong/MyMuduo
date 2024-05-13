#include "Logger.h"
#include "Timestamp.h"

#include <iostream>
// ��ȡ��־Ψһ��ʵ������,����,�̰߳�ȫ��
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}
// ������־����
void Logger::setLogLevel(int Level)
{
    logLevel_ = Level;
}
// д��־ [������Ϣ] time : msg
void Logger::log(std::string msg)
{
    // ��ӡ������Ϣ
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }
    
    // ��ӡʱ���msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}