#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// �൱���¼��ַ���Demultiplex

/**
 * ���������У�IO��·���ü���ĳ���ļ�������fd�������Ȥ���¼�
 * ��ͨ��epoll_ctlע�ᵽIO��·����ģ��Poller���¼����������ϡ�
 * ���¼����������������¼������ˣ��򷵻ء������¼���fd���ϡ��͡�ÿ��fd��������ʲô�¼�����
*/

// ���channelδ��ӵ�poller��
const int kNew = -1;  // channel�ĳ�Աindex_ = -1
// ���channel����ӵ�poller��
const int kAdded = 1;
// ���channel��poller��ɾ��
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop) // ͨ������Ĺ��캯���������Ա��ʼ��
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) // ��epoll��������
    , events_(kInitEventListSize)  // vector<epoll_event>��Ĭ�ϳ���16
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

// ͨ��epoll_wait�ѷ����¼���Channelͨ������activeChannels��֪��EventLoop
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // ʵ����Ӧ����LOG_DEBUG�����־��Ϊ������Ϊ�ýӿ��ڸ߲��������¶�ʱ���ڵ��÷ǳ�Ƶ���������־��Ӱ��Ч��
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    // ͨ��epoll_wait������ЩChannel�������¼�
    // �ڶ���������Ҫ��ŷ����¼���fd��enent�������ʼ��ַ
    // events_.begin()���ص���������Ԫ�صĵ��������ײ��Ǹ����飬������vector
    // *�����þ�����Ԫ�ص�ֵ��&��Ҫ�����Ԫ�ص��ڴ�ĵ�ַ���൱��vector�ײ��������ʼ��ַ
    // ������Ԫ��ʹ��ǿ��ת������Ϊevents_.size()���ص���size_t
    // ���һ�������ǳ�ʱʱ��
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    // poll�����ڶ���߳��ﶼ����ã���˶��EventLoop���п����ڵ�����epoll_wait�������дȫ�ֱ���errno
    // ��˵�����epoll_wait������һ���ֲ���������errno
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    // �������з����¼���Channel
    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        // ��������¼��ĸ�������events_����Ĵ�С����������
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0) // û���з����¼�
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else // ����
    {   
        // saveErrno == EINTR��ʾ�������ⲿ�жϣ���ô�������оͿ�����
        // ��������������������������
        if (saveErrno != EINTR)
        {
            errno = saveErrno; // ����errno
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/**
 *  EventLoop����ChannelList��Poller
 * 
 *              EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap => <fd, channel*>   epollfd
 * 
 *  һ��Channel��ʾһ��fd�����Channel��Pollerע����Ļ���
 *  �Ͱ�Channel�ĵ�ַ��¼��ChannelMap��
 *  ���ChannelList��Channel���������ڵ���ChannelMap��Channel������
 */ 
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index(); // 3��״̬
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    // Channelδ��ӻ�����ɾ��
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded); // �޸�Channel״̬
        update(EPOLL_CTL_ADD, channel); // ����epoll_ctl����epoll�����һ��Channel
    }
    else  // channel�Ѿ���poller��ע�����
    {
        int fd = channel->fd();
        // ���Channel���κ��¼���������Ȥ,��˲���Ҫepoll���м��������Դ�epoll��ɾ����
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel); // ����epoll_ctl����epoll��ɾ�����Channel
            channel->set_index(kDeleted); // ��Channel��index���ó���ɾ��
        }
        else // ���Channel�ж��¼�����Ȥ
        {
            update(EPOLL_CTL_MOD, channel); // ����epoll_ctl������ע��Channel����Ȥ�¼����и���
        }
    }
}

// ��poller��ɾ��channel������ChannelMap��ɾ�������Ҵ�epoll��ɾ��channel��������ChannelList��
void EPollPoller::removeChannel(Channel *channel) 
{
    int fd = channel->fd();
    channels_.erase(fd); // ��ChannelMap��ɾ��channel

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);
    
    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel); // ��epoll��ɾ��channel
    }
    channel->set_index(kNew);
}

// ��д��Ծ������
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i=0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr); // �����¼���channel
        channel->set_revents(events_[i].events); // �ѷ������¼�����channel
        activeChannels->push_back(channel); // EventLoop���õ�������poller�������ص����з����¼���channel�б�activeChannels��
    }
}

// ����channelͨ�� epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event); // ��event����
    
    int fd = channel->fd();

    event.events = channel->events(); // ����fd������Ȥ���¼�
    event.data.fd = fd; 
    event.data.ptr = channel; // ָ��Channel

    // ʹ��epoll_ctl��epoll��fd��ص��¼����и���
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) // epoll_ctl�����¼�ʧ��
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno); // ���Զ��˳�
        }
    }
}