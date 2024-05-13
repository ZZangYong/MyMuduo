#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>
class Channel; // ֻ�õ���ָ�����ͣ�����Ҫдͷ�ļ���ֻ������
class EventLoop; 
/**
 * Poller���ܿ���Ҳ����ʵ������ΪʲômuduoҪ����Poller?
 * ʵ��ʹ�õ���PollPoller.h��.cc(poll)��EpollPoller.h��.cc(epoll)
 * EventLoopʹ��epoll��ʱ����ֱ��ʹ��epoll
 * �����ڳ������ʹ�ó�����Poller��ͨ�����ò�ͬ�����������
 * �������ǵ�ͬ�����Ƿ������Ϳ��Է������չIO���÷��������Ƕ�·�ַ�����
*/

// muduo���ж�·�¼��ַ����ĺ���IO����ģ��
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *Loop);
    virtual ~Poller() = default;; // ����������

    // ������IO���ñ���ͳһ�Ľӿڣ�ʵ�ֲ�ͬIO���û���
    // ���¼�ѭ���߳�����ã������������ʱʱ��͵�ǰ�����Channels��Poller�չ˵�Channels��
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0; // Channel����ʱ���յ�ǰChannel��ָ��
    virtual void removeChannel(Channel *channel) = 0; // Channel����ʱ���յ�ǰChannel��ָ��

    // �жϲ���channel�Ƿ��ڵ�ǰPoller����
    bool hasChannel(Channel *channel) const;

    // EventLoop����ͨ���ýӿڻ�ȡĬ�ϵ�IO���õľ���ʵ��
    // ���Ƶ���ģʽ��ȡһ��Pollerʵ��
    // Ϊʲô�����̬������д��Poller.cc�
    // ��Ϊ���������Ҫ����һ�������IO���ö��󣬲�����һ��ָ��
    // �������Ҫ������PollPoller.h���͡�EpollPoller.h��
    // ���ܴ���poll����epoll��ʵ�������󣬻��������������࣬�������಻�����û���
    // �ڳ�������������������ͷ�ļ��ǲ��õ�ʵ�֣����Poller.ccû��newDefaultPoller��ʵ��
    // ����ʵ����һ��������Դ�ļ���DefaulltPoller.cc����
    static Poller* newDefaultPoller(EventLoop *loop);

protected: // protectedΪ�����������ܹ�����
    // map��key��sockfd  value��sockfd������channelͨ������
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_; // ����Poller�������¼�ѭ��EventLoop
};
