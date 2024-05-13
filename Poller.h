#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>
class Channel; // 只用到了指针类型，不需要写头文件，只需声明
class EventLoop; 
/**
 * Poller不能拷贝也不能实例化，为什么muduo要抽象Poller?
 * 实际使用的是PollPoller.h和.cc(poll)和EpollPoller.h和.cc(epoll)
 * EventLoop使用epoll的时候不是直接使用epoll
 * 而是在抽象层面使用抽象类Poller，通过引用不同的派生类对象
 * 调用他们的同名覆盖方法，就可以方便地扩展IO复用方法（就是多路分发器）
*/

// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *Loop);
    virtual ~Poller() = default;; // 虚析构函数

    // 给所有IO复用保留统一的接口，实现不同IO复用机制
    // 在事件循环线程里调用，输入参数：超时时间和当前激活的Channels（Poller照顾的Channels）
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0; // Channel调用时接收当前Channel的指针
    virtual void removeChannel(Channel *channel) = 0; // Channel调用时接收当前Channel的指针

    // 判断参数channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    // 类似单例模式获取一个Poller实例
    // 为什么这个静态方法不写在Poller.cc里？
    // 因为这个方法是要生成一个具体的IO复用对象，并返回一个指针
    // 因此他需要包含“PollPoller.h”和“EpollPoller.h”
    // 才能创建poll或者epoll的实例化对象，基类能引用派生类，而派生类不能引用基类
    // 在抽象基类里引用派生类的头文件是不好的实现，因此Poller.cc没有newDefaultPoller的实现
    // 它的实现在一个单独的源文件“DefaulltPoller.cc”中
    static Poller* newDefaultPoller(EventLoop *loop);

protected: // protected为了让派生类能够访问
    // map的key：sockfd  value：sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};
