#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

// �жϲ���channel�Ƿ��ڵ�ǰPoller����
bool Poller::hasChannel(Channel *channel) const
{
    // ͨ�����ҹ�ϣ���fd�������ٲ���channel�Ƿ��ڹ�ϣ��channels_��
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel; // it�ҵ�����channel��ȷ���true
}