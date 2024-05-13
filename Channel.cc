#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

// channel的tie方法什么时候调用过？
// 一个TcpConnection新连接创建的时候 TcpConnection => Channel 
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/**
 * 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 * Channel不能直接访问Poller，但他们都属于EventLoop,因此可以通过EventLoop做这件事
 * EventLoop => ChannelList   Poller
 */ 
void Channel::update()
{
    // 通过channel所属的EventLoop，调用poller的相应方法，注册当前通道fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中， 把当前的channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的
void Channel::handleEvent(Timestamp receiveTime)
{
    // 如果绑定过（TcpConnection新连接创建时绑定），说明tied_监听过当前Channel
    if (tied_)
    {
        // 把弱智能指针通过lock()提升成强智能指针
        std::shared_ptr<void> guard = tie_.lock();
        // 看当前Channel是否存活，存活才会执行回调
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        // 没有绑定过就直接处理
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件， 由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{   
    // 可以打印需要的日志消息
    // __FUNCTION__LINE__ 可以知道是哪个函数
    // __FUNCTION__FILE__ 可以知道是哪个文件
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    // 发生异常，关掉Channel
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // 如果发生错误，执行错误处理回调
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // 如果有可读事件发生，执行读操作回调
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // 如果有可写事件发生，执行写操作回调
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}
