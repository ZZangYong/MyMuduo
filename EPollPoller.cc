#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// 相当于事件分发器Demultiplex

/**
 * 在网络编程中，IO多路复用监听某个文件描述符fd和其感兴趣的事件
 * 并通过epoll_ctl注册到IO多路复用模块Poller（事件监听器）上。
 * 当事件监听器监听到有事件发生了，则返回“发生事件的fd集合”和“每个fd都发生了什么事件”。
*/

// 这个channel未添加到poller中
const int kNew = -1;  // channel的成员index_ = -1
// 这个channel已添加到poller中
const int kAdded = 1;
// 这个channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop) // 通过基类的构造函数给基类成员初始化
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) // 把epoll创建起来
    , events_(kInitEventListSize)  // vector<epoll_event>，默认长度16
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() 
{
    ::close(epollfd_);
}

// 通过epoll_wait把发生事件的Channel通过参数activeChannels告知给EventLoop
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际上应该用LOG_DEBUG输出日志更为合理，因为该接口在高并发场景下短时间内调用非常频繁，输出日志会影响效率
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    // 通过epoll_wait监听哪些Channel发生了事件
    // 第二个参数需要存放发生事件的fd的enent数组的起始地址
    // events_.begin()返回的是容器首元素的迭代器，底层是个数组，而不是vector
    // *解引用就是首元素的值，&是要存放首元素的内存的地址，相当于vector底层数组的起始地址
    // 第三个元素使用强制转换是因为events_.size()返回的是size_t
    // 最后一个参数是超时时间
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    // poll可能在多个线程里都会调用，因此多个EventLoop都有可能在调用完epoll_wait后出错，并写全局变量errno
    // 因此调用完epoll_wait后先用一个局部变量保存errno
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    // 监听到有发生事件的Channel
    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        // 如果发生事件的个数等于events_数组的大小，进行扩容
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0) // 没到有发生事件
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else // 出错
    {   
        // saveErrno == EINTR表示发生了外部中断，那么继续运行就可以了
        // 否则就是由其他错误类型引起的
        if (saveErrno != EINTR)
        {
            errno = saveErrno; // 重置errno
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/**
 *  EventLoop包含ChannelList和Poller
 * 
 *              EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap => <fd, channel*>   epollfd
 * 
 *  一个Channel表示一个fd，如果Channel向Poller注册过的话，
 *  就把Channel的地址记录在ChannelMap里
 *  因此ChannelList里Channel的数量大于等于ChannelMap里Channel的数量
 */ 
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index(); // 3个状态
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    // Channel未添加或者已删除
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded); // 修改Channel状态
        update(EPOLL_CTL_ADD, channel); // 调用epoll_ctl，往epoll中添加一个Channel
    }
    else  // channel已经在poller上注册过了
    {
        int fd = channel->fd();
        // 如果Channel对任何事件都不感兴趣,因此不需要epoll进行监听，可以从epoll中删除了
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel); // 调用epoll_ctl，从epoll中删除这个Channel
            channel->set_index(kDeleted); // 将Channel的index设置成已删除
        }
        else // 如果Channel有对事件感兴趣
        {
            update(EPOLL_CTL_MOD, channel); // 调用epoll_ctl，对已注册Channel感兴趣事件进行更改
        }
    }
}

// 从poller中删除channel，即从ChannelMap里删除掉，且从epoll中删除channel，但还在ChannelList里
void EPollPoller::removeChannel(Channel *channel) 
{
    int fd = channel->fd();
    channels_.erase(fd); // 从ChannelMap里删掉channel

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);
    
    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel); // 从epoll中删除channel
    }
    channel->set_index(kNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i=0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr); // 发生事件的channel
        channel->set_revents(events_[i].events); // 把发生的事件传给channel
        activeChannels->push_back(channel); // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表activeChannels了
    }
}

// 更新channel通道 epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event); // 把event清零
    
    int fd = channel->fd();

    event.events = channel->events(); // 返回fd所感兴趣的事件
    event.data.fd = fd; 
    event.data.ptr = channel; // 指向Channel

    // 使用epoll_ctl把epoll里fd相关的事件进行更改
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) // epoll_ctl更改事件失败
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno); // 会自动退出
        }
    }
}