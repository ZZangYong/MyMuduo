#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

// DefaultPoller.cc��һ��������Դ�ļ�,������������ϵû����
// ����Poller�����ϲ�ĳ�����࣬�����������������������ò�Ҫ����������
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    // �����ǰ�����˻�����������MUDUO_USE_POLL��ִ��
    if (::getenv("MUDUO_USE_POLL")) // ��ȡ��������
    {
        return nullptr; // ����poll��ʵ��
    }
    else // Ĭ��û�����û�����������MUDUO_USE_POLL�����Ĭ��ʹ��EPOLL
    {
        return new EPollPoller(loop); // ����epoll��ʵ��
    }
}