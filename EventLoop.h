#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// �¼�ѭ���࣬��Ҫ������������ģ��Channel��Poller(epoll�ĳ���)
// ������һ��Poller��һ��Channel�б�
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>; // ����һ���ص�����

    EventLoop();
    ~EventLoop();

    // �����¼�ѭ�� ��?�ú������̱߳����Ǹ� EventLoop �����̣߳�Ҳ���� Loop �������ܿ��̵߳�?
    void loop();
    // �˳��¼�ѭ��
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // �ڵ�ǰloop��ִ�лص�cb �����ǰ�߳̾��Ǵ�����EventLoop���߳� �͵�?cb ����ͷ�?�ȴ�ִ?��������
    void runInLoop(Functor cb);
    // ��cb��������У�ֱ���ֵ�����loop���ڵ��̣߳���ִ�лص�cb
    void queueInLoop(Functor cb);

    // mianReactor�첽����SubLoop��epoll_wait(��event_fd��д?����)
    void wakeup();

    // EventLoop�ķ��� =�� Poller�ķ���
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // �ж�EventLoop�����Ƿ����Լ����߳�����
    bool isInLoopThread() const { return threadId_ ==  CurrentThread::tid(); }
private:
    void handleRead(); // wake up
    void doPendingFunctors(); // ִ�лص�

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_; // ԭ�Ӳ�����ͨ��CASʵ�ֵ�
    std::atomic_bool quit_; // ��ʶ�˳�loopѭ��

    const pid_t threadId_; // ��¼��ǰloop�����̵߳�id

    Timestamp pollReturnTime_; // poller���ط����¼���channels��ʱ���
    std::unique_ptr<Poller> poller_; // io��·��? �ַ�������̬������Դ

     // ��Ҫ���ã���mainLoop��ȡһ�����û���channel��ͨ����ѯ�㷨ѡ��һ��subloop
     // ͨ���ó�ԱwakeupFd_����subloop����channel
    int wakeupFd_; // ʹ��eventfd()ϵͳ���ô����ģ�?���첽���� SubLoop �� Loop �����е�Poll(epoll_wait��Ϊ��û��ע��fd��?ֱ����)
    std::unique_ptr<Channel> wakeupChannel_; // ?���첽���ѵ� channel

    ChannelList activeChannels_; // EventLoop�����������Channel
    // Channel* currentActiveChannel_; // ���ڶ��Բ���

    std::atomic_bool callingPendingFunctors_; // ��ʶ��ǰloop�Ƿ�����Ҫִ�еĻص�����
    std::vector<Functor> pendingFunctors_; // �洢loop��Ҫִ�е����еĻص�����
    std::mutex mutex_; // ��������������������vector�������̰߳�ȫ����
};