#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr) // ���ָ��Ϊ�գ�ֻ���˳�
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop; // ָ�벻�գ�����loop
}

TcpServer::TcpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option)
                : loop_(CheckLoopNotNull(loop))  // �������û�����ָ��
                , ipPort_(listenAddr.toIpPort())
                , name_(nameArg)
                , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
                , threadPool_(new EventLoopThreadPool(loop, name_)) // �½��̳߳أ�����û�п������̣߳�Ĭ��ֻ�����߳�
                , connectionCallback_()
                , messageCallback_()
                , nextConnId_(1)
                , started_(0)
{
    // �������û�����ʱ����ִ��TcpServer::newConnection�ص�
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, 
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        // ����ֲ���shared_ptr����ָ����󣬳������ţ������Զ��ͷ�new������TcpConnection������Դ��
        TcpConnectionPtr conn(item.second); 
        item.second.reset(); // ������ǿ����ָ�������Դ����ǿ����ָ�������Դ�޷��ͷ���Դ

        // ��������
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

// ���õײ�subloop�ĸ���
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// ��������������   loop.loop()
void TcpServer::start()
{
    if (started_++ == 0) // ��ֹһ��TcpServer����start��Σ���һ�ε��õ�ʱ��started_=0��֮��ʹ���0
    {
        threadPool_->start(threadInitCallback_); // �����ײ��loop�̳߳�
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); // ��acceptChannel_ע�ᵽmainReactor��Poller��
    }
}

// ��һ���µĿͻ��˵����ӣ�acceptor��ִ������ص�����
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // ��ѯ�㷨��ѡ��һ��subLoop��������channel
    EventLoop *ioLoop = threadPool_->getNextLoop(); 
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // ͨ��sockfd��ȡ��󶨵ı�����ip��ַ�Ͷ˿���Ϣ
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // �������ӳɹ���sockfd������TcpConnection���Ӷ���
    TcpConnectionPtr conn(new TcpConnection(
                            ioLoop,
                            connName,
                            sockfd,   // ͨ��sockfd����Socket��Channel
                            localAddr,
                            peerAddr));
    connections_[connName] = conn;
    // ����Ļص������û����ø�TcpServer=>TcpConnection=>Channel=>Poller=>notify channel���ûص�
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // ��������ιر����ӵĻص�   conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );

    // ֱ�ӵ���TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
        name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name()); // �����Ӵ�ConnectionMap��ɾ��
    EventLoop *ioLoop = conn->getLoop(); // ��ȡConn���ڵ�loop
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
