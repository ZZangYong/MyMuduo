#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; // �̳߳�ʼ���ص�
    
    // ThreadInitCallback()û�д�EventLoop*�������൱��ʲôҲû����
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), 
        const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop(); // ��ʼѭ��
private:
    void threadFunc(); // �̺߳���

    EventLoop *loop_; // �뻥�������
    bool exiting_; // �Ƿ��˳�
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_; // �뻥�������
    ThreadInitCallback callback_;
};