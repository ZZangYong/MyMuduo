#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类，主要包含了两个大模块Channel和Poller(epoll的抽象)
// 包含了一个Poller和一个Channel列表
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>; // 定义一个回调类型

    EventLoop();
    ~EventLoop();

    // 开启事件循环 调?该函数的线程必须是该 EventLoop 所在线程，也就是 Loop 函数不能跨线程调?
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行回调cb 如果当前线程就是创建此EventLoop的线程 就调?cb 否则就放?等待执?函数队列
    void runInLoop(Functor cb);
    // 把cb放入队列中，直到轮到唤醒loop所在的线程，才执行回调cb
    void queueInLoop(Functor cb);

    // mianReactor异步唤醒SubLoop的epoll_wait(向event_fd中写?数据)
    void wakeup();

    // EventLoop的方法 =》 Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ ==  CurrentThread::tid(); }
private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_; // 原子操作，通过CAS实现的
    std::atomic_bool quit_; // 标识退出loop循环

    const pid_t threadId_; // 记录当前loop所在线程的id

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_; // io多路复? 分发器，动态管理资源

     // 主要作用，当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop
     // 通过该成员wakeupFd_唤醒subloop处理channel
    int wakeupFd_; // 使用eventfd()系统调用创建的，?于异步唤醒 SubLoop 的 Loop 函数中的Poll(epoll_wait因为还没有注册fd会?直阻塞)
    std::unique_ptr<Channel> wakeupChannel_; // ?于异步唤醒的 channel

    ChannelList activeChannels_; // EventLoop所管理的所有Channel
    // Channel* currentActiveChannel_; // 用在断言操作

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有的回调操作
    std::mutex mutex_; // 互斥锁，用来保护上面vector容器的线程安全操作
};