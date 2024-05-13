#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, 
        const std::string &name)
        : loop_(nullptr) // �մ����̣߳���û��loop
        , exiting_(false) // ��û��ʼѭ��
        , thread_(std::bind(&EventLoopThread::threadFunc, this), name) // ���̺߳���
        , mutex_() // ��������Ĭ�Ϲ���
        , cond_() // ����������Ĭ�Ϲ���
        , callback_(cb) // �̳߳�ʼ���ص�����
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true; // �˳���
    if (loop_ != nullptr) // �¼�ѭ����Ϊ��
    {
        loop_->quit(); // ���̰߳󶨵��¼�ѭ���˳�
        thread_.join(); // �ȴ��ײ����߳̽���
    }
}

EventLoop* EventLoopThread::startLoop()
{
    // �����ײ�����̣߳�ÿ��thread_.start();��������һ�����߳�
    // ���߳�ִ�е��ǰ󶨸�thread_���̺߳���EventLoopThread::threadFunc
    thread_.start(); 

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while ( loop_ == nullptr ) // �´��������̻߳�û��ִ����loop_ = &loop; ����֪ͨ
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// ����������������ڵ��������߳��������е�
void EventLoopThread::threadFunc()
{
    EventLoop loop; // ����һ��������eventloop����������߳���һһ��Ӧ�ģ�one loop per thread

    if (callback_) // ������ݹ�callback_�����ȵ��ûص�
    {
        callback_(&loop);
    }

    {
        // ���ܶ�loop_ͬʱ�����޸�
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop; 
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop  => Poller.poll ��������״̬����Զ���û������ӻ����������û��Ķ�д�¼�
    // һ���ǲ����loop()������ģ������Ļ����Ƿ�����Ҫ�����¼�ѭ����
    std::unique_lock<std::mutex> lock(mutex_); // �������б���
    loop_ = nullptr;
}