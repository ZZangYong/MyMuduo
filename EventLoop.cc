#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 相当于Reactor

// EventLoop指针变量，指向一个EventLoop对象，防止一个线程创建多个EventLoop   
// __thread用于让每个线程都有一个副本，而不是全局变量   thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间 10s
const int kPollTimeMs = 10000;

// 调用系统函数eventfd，创建wakeupfd，用来notify唤醒subReactor处理新来的channel
// mainReactor通过轮询的方式唤醒subReactor
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // 非阻塞
    if (evtfd < 0) // 出错
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
    if (t_loopInThisThread) // 该线程已经有一个Loop
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else // 该线程没有Loop
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听自己的wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // 设置对任何事件不感兴趣
    wakeupChannel_->remove(); // 在channel所属的EventLoop中， 把当前的channel删除掉
    ::close(wakeupFd_); // 关闭文件描述符wakeupFd_
    t_loopInThisThread = nullptr; // 指针置空
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true; // 标识开启事件循环
    quit_ = false; // 标识未退出

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        // Poller监听哪些channel发生事件了，然后上报给EventLoop
        // 监听两类fd   一种是client的fd，一种wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // EventLoop通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }

        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * 例子：IO线程即mainLoop，做accept接收新用户连接的工作，返回跟客户端做通信的fd
         * 然后用channel打包fd,并分发给subloop
         * mainLoop 事先注册一个回调cb（需要subloop来执行）
         * wakeup subloop后，执行doPendingFunctors()，执行之前mainloop注册的一个或多个cb回调操作
         */ 
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环
// 1.loop在自己的线程中调用quit，此时肯定不在poll()里
// 2.在非loop的线程中，调用loop的quit
//   例如在一个subloop(woker线程)中调用了mainloop(IO线程)的quit
//   那么你调用别的线程的loop，就应该先把别人唤醒，此时就从poll()返回回来了
/**
 *              mainLoop
 * 
 *                                             no ==================== 生产者-消费者的线程安全的队列
 * 
 *  subLoop1     subLoop2     subLoop3
 */ 
void EventLoop::quit()
{
    quit_ = true;

    // 如果是在其它线程中，调用的quit   在一个subloop(woker)中，调用了mainLoop(IO)的quit
    if (!isInLoopThread())  
    {
        wakeup(); // 唤醒其他线程
    }
}

// 在当前loop中执行回调cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 在当前的loop线程中，执行回调cb
    {
        cb();
    }
    else // 在非当前loop线程中执行cb , 就需要唤醒对应loop所在线程，执行回调cb
    {
        queueInLoop(cb);
    }
}
// 把cb放入队列中，唤醒对应loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    // 因为其他线程可能对pendingFunctors_有并发操作，因此需要加锁保护它的线程安全
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // emplace_back()是直接构造（推荐），push_back()是拷贝构造
    }

    // 唤醒相应的，需要执行上面回调操作的loop的线程了
    // 两种情况：1、!isInLoopThread()：确实不在当前线程执行回调，因此需要唤醒对应loop所在线程
    // 2、|| callingPendingFunctors_：在当前loop正在doPendingFunctors()执行回调，但是当前loop又被写入了新的回调
    // 因此需要等loop执行完当前回调，到下一个循环进入poll()时唤醒继续执行新的回调
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // 唤醒loop所在线程
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one); // 系统函数read
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// 用来唤醒loop所在的线程的  向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法 =》 Poller的方法
// Channel通过父组件EventLoop跟Poller沟通
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

// 执行回调
void EventLoop::doPendingFunctors() 
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 把pendingFunctors_和局部变量functors交换，相当于把pendingFunctors_置空，把待执行的回调放到functors里
        // 如果不是交换而是从pendingFunctors_里一个个拿，可能会导致当mianloop无法及时向subloop注册新的回调
        // 使得mianloop阻塞在下发Channel这一步上，使得服务器时延变长
        functors.swap(pendingFunctors_); 
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}