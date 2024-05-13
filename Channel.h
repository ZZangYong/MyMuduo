#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop; // 类型的前置声明
// EventLoop相当于多路事件分发器，一个线程一个（one loop per thread）
// 一个EventLoop包含了一个Poller和一组Channel（一个线程可以监听多路Channel，多路fd）
// Channel里包含了fd和其感兴趣的event和最终发生的事件
// 这些事件最终要向Poller里注册，发生的事件由Poller给Channel通知
// Channel得到相应fd的通知后，调用预制的回调操作
/*
Channel理解为通道，封装了sockfd和其感兴趣的event,如EPOLLIN、EPOLLOUT事件
还绑定了poller返回的具体事件。
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>; // 事件的回调
    using ReadEventCallback = std::function<void(Timestamp)>; // 只读事件的回调

    Channel(EventLoop *Loop, int fd); // 封装了sockfd和其感兴趣的event
    ~Channel();

    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb);} // 把左值转成右值
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    // 防止当channel被手动remove掉，channel还在执行回调操作
    // 使用弱指针tie_监听channel是否被手动remove掉
    void tie(const std::shared_ptr<void>&);
    // 返回成员变量的值
    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revent(int revt) {revents_ = revt;}
    
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    // 把channel的所有感兴趣的事件，从poller中del掉
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // 跟业务强相关，跟逻辑相关性不强
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    void set_revents(int revt) { revents_ = revt; }

    // 一个线程一个Loop : one loop per thread
    EventLoop* ownerLoop() {return loop_;}
    
    // 删除Channel
    void remove();
private:
    void update(); // 在poller里面更改fd相应的事件epoll_ctl()
    void handleEventWithGuard(Timestamp receiveTime); // 受保护的处理事件

    static const int kNoneEvent; // 没有对任何事件感兴趣
    static const int kReadEvent; // 对读事件感兴趣
    static const int kWriteEvent; // 对写事件感兴趣

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd,Poller监听的对象
    int events_; // 注册fd感兴趣的事件
    int revents_; // poller返回的实际具体发生的事件类型（epoll或者poll）
    int index_; // Poller用的

    std::weak_ptr<void> tie_; // 观察一个强智能指针，绑定自己，也可以用shared_from_this()来获得指向自身的shared_ptr
    bool tied_; // 是否绑定强智能指针shared_ptr

    // 因为Channel通道里面能够获知fd最终发生的具体的事件revents,所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};