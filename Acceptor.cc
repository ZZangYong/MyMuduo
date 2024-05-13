#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking() // ��static��ֹ�����ļ��ĺ�������
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) // ����������listenfd�׽���acceptSocket_
    , acceptChannel_(loop, acceptSocket_.fd()) // ���׽���acceptSocket_��װ��acceptChannel_�У�����acceptChannel_���䵽loop��
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // ��Ҫ������ip�Ͷ˿ڰ󶨵��׽�����
    // TcpServer::start()����Acceptor.listen()
    // Acceptor������������������û������ӣ�Ҫִ��һ���ص����ͻ���connfd=��channel=��subloop��
    
    // baseLoop������listenfd���¼�������ִ�лص�����Acceptor::handleRead(ֻ���Ķ��¼�)
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); // ��Channel��baseLoop���Poller�������������Poller��ע���д�¼���
    acceptChannel_.remove(); // ���Լ���Poller��ɾ����
}

void Acceptor::listen()
{
    listenning_ = true; // ��ʼ����
    acceptSocket_.listen(); // ����listen����
    acceptChannel_.enableReading(); // ��acceptChannel_ע�ᵽPoller��
}

// listenfd���¼������ˣ����������û������ˣ�ִ�лص�����Acceptor::handleRead()
void Acceptor::handleRead()
{
    InetAddress peerAddr; // �ͻ�������
    int connfd = acceptSocket_.accept(&peerAddr); // ����accept()�������������û�����ͨ�ŵ�socket��fd
    if (connfd >= 0) // ִ�лص�
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr); // ��ѯ�ҵ�subLoop�����ѣ��ַ���ǰ���¿ͻ��˵�Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else // �����ˣ���ӡ��־
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE) // �ļ���������������
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}