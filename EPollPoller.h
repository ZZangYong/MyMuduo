#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/**
 * epoll的使用  
 * epoll_create：创建epoll的fd
 * epoll_ctl: 添加想让epoll监听的fd，并针对该fd感兴趣的事件：add/mod/del事件
 * epoll_wait
 */ 
class EPollPoller : public Poller
{
public:
    // 构造和析构对应epoll_create
    EPollPoller(EventLoop *loop); // 创建epoll的fd，记录在epollfd_中
    ~EPollPoller() override; // override:重写虚析构函数；关闭epoll的fd

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override; // 对应epoll_wait
    void updateChannel(Channel *channel) override; // 对应epoll_ctl
    void removeChannel(Channel *channel) override; // 对应epoll_ctl
private:
    static const int kInitEventListSize = 16; // 给EventList初始的常量大小

    // 被public里的方法所调用，封装了一些模块化的操作
    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_; // 存储发生了的事件和对应Channel信息,epoll_wait()的第二个参数，默认长度是kInitEventListSize = 16
};