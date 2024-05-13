#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/**
 * epoll��ʹ��  
 * epoll_create������epoll��fd
 * epoll_ctl: �������epoll������fd������Ը�fd����Ȥ���¼���add/mod/del�¼�
 * epoll_wait
 */ 
class EPollPoller : public Poller
{
public:
    // �����������Ӧepoll_create
    EPollPoller(EventLoop *loop); // ����epoll��fd����¼��epollfd_��
    ~EPollPoller() override; // override:��д�������������ر�epoll��fd

    // ��д����Poller�ĳ��󷽷�
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override; // ��Ӧepoll_wait
    void updateChannel(Channel *channel) override; // ��Ӧepoll_ctl
    void removeChannel(Channel *channel) override; // ��Ӧepoll_ctl
private:
    static const int kInitEventListSize = 16; // ��EventList��ʼ�ĳ�����С

    // ��public��ķ��������ã���װ��һЩģ�黯�Ĳ���
    // ��д��Ծ������
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // ����channelͨ��
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_; // �洢�����˵��¼��Ͷ�ӦChannel��Ϣ,epoll_wait()�ĵڶ���������Ĭ�ϳ�����kInitEventListSize = 16
};