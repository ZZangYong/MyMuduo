#pragma once

#include "noncopyable.h"

class InetAddress;

// ��װsocket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}

    ~Socket(); // �ر�fd

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr); 
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();
    // ����TCPѡ��
    void setTcpNoDelay(bool on); // ֱ�ӷ��ͣ������ݲ�����TCP����
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};