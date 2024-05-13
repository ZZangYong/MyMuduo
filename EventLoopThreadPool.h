#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; // 线程初始化回调类型

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
    // 设置底层线程数量
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    // 开启整个事件循环线程
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    // 如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }
private:
    EventLoop *baseLoop_; // EventLoop loop;用户使用的线程，如果不设置线程数量，默认就是单线程，负责连接和读写
    std::string name_;
    bool started_; // 是否启动
    int numThreads_; // 线程数量
    int next_; // 做轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 包含了所有创建了事件的线程
    std::vector<EventLoop*> loops_; // 包含了事件线程里EventLoop的指针（通过调用EventLoopThread::startLoop()获得）
};