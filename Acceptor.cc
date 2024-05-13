#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking() // 用static防止与别的文件的函数重名
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) // 创建非阻塞listenfd套接字acceptSocket_
    , acceptChannel_(loop, acceptSocket_.fd()) // 将套接字acceptSocket_封装到acceptChannel_中，并把acceptChannel_分配到loop中
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // 把要监听的ip和端口绑定到套接字上
    // TcpServer::start()启动Acceptor.listen()
    // Acceptor运行起来后，如果有新用户的连接，要执行一个回调（客户端connfd=》channel=》subloop）
    
    // baseLoop监听到listenfd有事件发生，执行回调函数Acceptor::handleRead(只关心读事件)
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); // 将Channel从baseLoop里的Poller清除掉，不再向Poller里注册读写事件了
    acceptChannel_.remove(); // 将自己从Poller里删除掉
}

void Acceptor::listen()
{
    listenning_ = true; // 开始监听
    acceptSocket_.listen(); // 启动listen监听
    acceptChannel_.enableReading(); // 将acceptChannel_注册到Poller里
}

// listenfd有事件发生了，就是有新用户连接了，执行回调函数Acceptor::handleRead()
void Acceptor::handleRead()
{
    InetAddress peerAddr; // 客户端连接
    int connfd = acceptSocket_.accept(&peerAddr); // 调用accept()函数，返回新用户用于通信的socket的fd
    if (connfd >= 0) // 执行回调
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr); // 轮询找到subLoop，唤醒，分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else // 出错了，打印日志
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE) // 文件描述符到达上限
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}