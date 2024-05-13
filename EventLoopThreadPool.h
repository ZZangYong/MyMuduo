#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; // �̳߳�ʼ���ص�����

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
    // ���õײ��߳�����
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    // ���������¼�ѭ���߳�
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    // ��������ڶ��߳��У�baseLoop_Ĭ������ѯ�ķ�ʽ����channel��subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }
private:
    EventLoop *baseLoop_; // EventLoop loop;�û�ʹ�õ��̣߳�����������߳�������Ĭ�Ͼ��ǵ��̣߳��������ӺͶ�д
    std::string name_;
    bool started_; // �Ƿ�����
    int numThreads_; // �߳�����
    int next_; // ����ѯ���±�
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // ���������д������¼����߳�
    std::vector<EventLoop*> loops_; // �������¼��߳���EventLoop��ָ�루ͨ������EventLoopThread::startLoop()��ã�
};