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
    using ThreadInitCallback = std::function<void(EventLoop*)>; // 线程初始化回调
    
    // ThreadInitCallback()没有传EventLoop*参数，相当于什么也没有做
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), 
        const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop(); // 开始循环
private:
    void threadFunc(); // 线程函数

    EventLoop *loop_; // 与互斥锁相关
    bool exiting_; // 是否退出
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_; // 与互斥锁相关
    ThreadInitCallback callback_;
};