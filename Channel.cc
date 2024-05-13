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

// channel��tie����ʲôʱ����ù���
// һ��TcpConnection�����Ӵ�����ʱ�� TcpConnection => Channel 
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/**
 * ���ı�channel����ʾfd��events�¼���update������poller�������fd��Ӧ���¼�epoll_ctl
 * Channel����ֱ�ӷ���Poller�������Ƕ�����EventLoop,��˿���ͨ��EventLoop�������
 * EventLoop => ChannelList   Poller
 */ 
void Channel::update()
{
    // ͨ��channel������EventLoop������poller����Ӧ������ע�ᵱǰͨ��fd��events�¼�
    loop_->updateChannel(this);
}

// ��channel������EventLoop�У� �ѵ�ǰ��channelɾ����
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd�õ�poller֪ͨ�Ժ󣬴����¼���
void Channel::handleEvent(Timestamp receiveTime)
{
    // ����󶨹���TcpConnection�����Ӵ���ʱ�󶨣���˵��tied_��������ǰChannel
    if (tied_)
    {
        // ��������ָ��ͨ��lock()������ǿ����ָ��
        std::shared_ptr<void> guard = tie_.lock();
        // ����ǰChannel�Ƿ�����Ż�ִ�лص�
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        // û�а󶨹���ֱ�Ӵ���
        handleEventWithGuard(receiveTime);
    }
}

// ����poller֪ͨ��channel�����ľ����¼��� ��channel������þ���Ļص�����
void Channel::handleEventWithGuard(Timestamp receiveTime)
{   
    // ���Դ�ӡ��Ҫ����־��Ϣ
    // __FUNCTION__LINE__ ����֪�����ĸ�����
    // __FUNCTION__FILE__ ����֪�����ĸ��ļ�
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    // �����쳣���ص�Channel
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // �����������ִ�д�����ص�
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // ����пɶ��¼�������ִ�ж������ص�
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // ����п�д�¼�������ִ��д�����ص�
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}
