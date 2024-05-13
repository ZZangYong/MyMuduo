#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// �൱��Reactor

// EventLoopָ�������ָ��һ��EventLoop���󣬷�ֹһ���̴߳������EventLoop   
// __thread������ÿ���̶߳���һ��������������ȫ�ֱ���   thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// ����Ĭ�ϵ�Poller IO���ýӿڵĳ�ʱʱ�� 10s
const int kPollTimeMs = 10000;

// ����ϵͳ����eventfd������wakeupfd������notify����subReactor����������channel
// mainReactorͨ����ѯ�ķ�ʽ����subReactor
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // ������
    if (evtfd < 0) // ����
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    // , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) // ���߳��Ѿ���һ��Loop
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else // ���߳�û��Loop
    {
        t_loopInThisThread = this;
    }

    // ����wakeupfd���¼������Լ������¼���Ļص�����
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // ÿһ��eventloop���������Լ���wakeupchannel��EPOLLIN���¼���
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // ���ö��κ��¼�������Ȥ
    wakeupChannel_->remove(); // ��channel������EventLoop�У� �ѵ�ǰ��channelɾ����
    ::close(wakeupFd_); // �ر��ļ�������wakeupFd_
    t_loopInThisThread = nullptr; // ָ���ÿ�
}

// �����¼�ѭ��
void EventLoop::loop()
{
    looping_ = true; // ��ʶ�����¼�ѭ��
    quit_ = false; // ��ʶδ�˳�

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        // Poller������Щchannel�����¼��ˣ�Ȼ���ϱ���EventLoop
        // ��������fd   һ����client��fd��һ��wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // EventLoop֪ͨchannel������Ӧ���¼�
            channel->handleEvent(pollReturnTime_);
        }

        // ִ�е�ǰEventLoop�¼�ѭ����Ҫ����Ļص�����
        /**
         * ���ӣ�IO�̼߳�mainLoop����accept�������û����ӵĹ��������ظ��ͻ�����ͨ�ŵ�fd
         * Ȼ����channel���fd,���ַ���subloop
         * mainLoop ����ע��һ���ص�cb����Ҫsubloop��ִ�У�
         * wakeup subloop��ִ��doPendingFunctors()��ִ��֮ǰmainloopע���һ������cb�ص�����
         */ 
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// �˳��¼�ѭ��
// 1.loop���Լ����߳��е���quit����ʱ�϶�����poll()��
// 2.�ڷ�loop���߳��У�����loop��quit
//   ������һ��subloop(woker�߳�)�е�����mainloop(IO�߳�)��quit
//   ��ô����ñ���̵߳�loop����Ӧ���Ȱѱ��˻��ѣ���ʱ�ʹ�poll()���ػ�����
/**
 *              mainLoop
 * 
 *                                             no ==================== ������-�����ߵ��̰߳�ȫ�Ķ���
 * 
 *  subLoop1     subLoop2     subLoop3
 */ 
void EventLoop::quit()
{
    quit_ = true;

    // ������������߳��У����õ�quit   ��һ��subloop(woker)�У�������mainLoop(IO)��quit
    if (!isInLoopThread())  
    {
        wakeup(); // ���������߳�
    }
}

// �ڵ�ǰloop��ִ�лص�cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // �ڵ�ǰ��loop�߳��У�ִ�лص�cb
    {
        cb();
    }
    else // �ڷǵ�ǰloop�߳���ִ��cb , ����Ҫ���Ѷ�Ӧloop�����̣߳�ִ�лص�cb
    {
        queueInLoop(cb);
    }
}
// ��cb��������У����Ѷ�Ӧloop���ڵ��̣߳�ִ��cb
void EventLoop::queueInLoop(Functor cb)
{
    // ��Ϊ�����߳̿��ܶ�pendingFunctors_�в��������������Ҫ�������������̰߳�ȫ
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // emplace_back()��ֱ�ӹ��죨�Ƽ�����push_back()�ǿ�������
    }

    // ������Ӧ�ģ���Ҫִ������ص�������loop���߳���
    // ���������1��!isInLoopThread()��ȷʵ���ڵ�ǰ�߳�ִ�лص��������Ҫ���Ѷ�Ӧloop�����߳�
    // 2��|| callingPendingFunctors_���ڵ�ǰloop����doPendingFunctors()ִ�лص������ǵ�ǰloop�ֱ�д�����µĻص�
    // �����Ҫ��loopִ���굱ǰ�ص�������һ��ѭ������poll()ʱ���Ѽ���ִ���µĻص�
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // ����loop�����߳�
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one); // ϵͳ����read
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// ��������loop���ڵ��̵߳�  ��wakeupfd_дһ�����ݣ�wakeupChannel�ͷ������¼�����ǰloop�߳̾ͻᱻ����
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop�ķ��� =�� Poller�ķ���
// Channelͨ�������EventLoop��Poller��ͨ
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

// ִ�лص�
void EventLoop::doPendingFunctors() 
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // ��pendingFunctors_�;ֲ�����functors�������൱�ڰ�pendingFunctors_�ÿգ��Ѵ�ִ�еĻص��ŵ�functors��
        // ������ǽ������Ǵ�pendingFunctors_��һ�����ã����ܻᵼ�µ�mianloop�޷���ʱ��subloopע���µĻص�
        // ʹ��mianloop�������·�Channel��һ���ϣ�ʹ�÷�����ʱ�ӱ䳤
        functors.swap(pendingFunctors_); 
    }

    for (const Functor &functor : functors)
    {
        functor(); // ִ�е�ǰloop��Ҫִ�еĻص�����
    }

    callingPendingFunctors_ = false;
}