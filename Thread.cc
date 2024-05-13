#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

//static thread_local Thread *t_thread          = nullptr;

std::atomic_int Thread::numCreated_(0);

// Thread *Thread::GetThis() {
//     return t_thread;
// }

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}

void Thread::start()  // 一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem; // 信号量
    sem_init(&sem, false, 0); // 初始化信号量资源为0

    // 开启子线程（lambda表达式）
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 子线程获取新创建的线程的tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem); // 信号量资源+1
        // 开启一个新线程，专门执行该线程函数
        func_(); 
    }));

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem); // 信号量没有资源的时候阻塞
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

// 按线程创建数定义默认名字
void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}