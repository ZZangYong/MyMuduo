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
        thread_->detach(); // thread���ṩ�����÷����̵߳ķ���
    }
}

void Thread::start()  // һ��Thread���󣬼�¼�ľ���һ�����̵߳���ϸ��Ϣ
{
    started_ = true;
    sem_t sem; // �ź���
    sem_init(&sem, false, 0); // ��ʼ���ź�����ԴΪ0

    // �������̣߳�lambda���ʽ��
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // ���̻߳�ȡ�´������̵߳�tidֵ
        tid_ = CurrentThread::tid();
        sem_post(&sem); // �ź�����Դ+1
        // ����һ�����̣߳�ר��ִ�и��̺߳���
        func_(); 
    }));

    // �������ȴ���ȡ�����´������̵߳�tidֵ
    sem_wait(&sem); // �ź���û����Դ��ʱ������
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

// ���̴߳���������Ĭ������
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