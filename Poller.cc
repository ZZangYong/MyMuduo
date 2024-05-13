#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

// 判断参数channel是否在当前Poller当中
bool Poller::hasChannel(Channel *channel) const
{
    // 通过查找哈希表的fd键，快速查找channel是否在哈希表channels_里
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel; // it找到了且channel相等返回true
}