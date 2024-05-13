#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>    
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    close(sockfd_); // �ر�fd
}

// ��װ�˰󶨲���
void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in))) // ֻ֧��IPv4
    {
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
}

// ��װ��listen()
void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}

// �������û�ͨ�ŵ�fd������Socket��ַ������������peeraddr��
int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 1. accept�����Ĳ������Ϸ�
     * 2. �Է��ص�connfdû�����÷�����
     * Reactorģ�� one loop per thread
     * poller + non-blocking IO
     */ 
    sockaddr_in addr;
    socklen_t len = sizeof addr; // ��Ҫ��ʼ��
    bzero(&addr, sizeof addr);
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);  // ���û�ͨ�ŵ�fd ������SOCK_NONBLOCK
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

// �ر�д��
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

// ֱ�ӷ��ͣ������ݲ�����TCP����
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0; // 1�Ǵ򿪣�0�ǹر�
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}
// ���ÿ����õ�ַ
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
// ���ÿ����ö˿�
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}