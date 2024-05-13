#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, 
        const std::string &name)
        : loop_(nullptr) // 刚创建线程，还没有loop
        , exiting_(false) // 还没开始循环
        , thread_(std::bind(&EventLoopThread::threadFunc, this), name) // 绑定线程函数
        , mutex_() // 互斥锁，默认构造
        , cond_() // 条件变量，默认构造
        , callback_(cb) // 线程初始化回调函数
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true; // 退出了
    if (loop_ != nullptr) // 事件循环不为空
    {
        loop_->quit(); // 把线程绑定的事件循环退出
        thread_.join(); // 等待底层子线程结束
    }
}

EventLoop* EventLoopThread::startLoop()
{
    // 启动底层的新线程，每次thread_.start();都会启动一个新线程
    // 新线程执行的是绑定给thread_的线程函数EventLoopThread::threadFunc
    thread_.start(); 

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while ( loop_ == nullptr ) // 新创建的子线程还没有执行完loop_ = &loop; 进行通知
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 下面这个方法，是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的eventloop，和上面的线程是一一对应的，one loop per thread

    if (callback_) // 如果传递过callback_，就先调用回调
    {
        callback_(&loop);
    }

    {
        // 不能对loop_同时进行修改
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop; 
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop  => Poller.poll 进入阻塞状态监听远端用户的连接或者已连接用户的读写事件
    // 一般是不会从loop()里出来的，出来的话就是服务器要结束事件循环了
    std::unique_lock<std::mutex> lock(mutex_); // 用锁进行保护
    loop_ = nullptr;
}