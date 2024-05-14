#pragma once

/**
 * �û�ʹ��muduo��д����������
 */ 
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

// ����ķ��������ʹ�õ���
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; // �̳߳�ʼ���ص����߳�
    
    // Ԥ������ѡ���ʾ�Ƿ�����ö˿�
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option = kNoReusePort); // Ĭ���ǲ����ö˿�
    ~TcpServer();

    // ���ûص�����
    void setThreadInitcallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // ���õײ�subloop�ĸ���
    void setThreadNum(int numThreads);

    // ����������������������Acceptor��listen()
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn); // ��ConnectionMap���Ƴ�����
    void removeConnectionInLoop(const TcpConnectionPtr &conn); 

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // baseLoop �û������loop

    const std::string ipPort_; // IP��Port
    const std::string name_; // ����

    std::unique_ptr<Acceptor> acceptor_; // ������mainLoop��������Ǽ����������¼�
    std::unique_ptr<EventLoopThreadPool> threadPool_; // �̳߳أ�one loop per thread

    ConnectionCallback connectionCallback_; // ��������ʱ�Ļص�
    MessageCallback messageCallback_; // �ж�д��Ϣʱ�Ļص�
    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ��������Ժ�Ļص�

    ThreadInitCallback threadInitCallback_; // loop�̳߳�ʼ���Ļص�

    std::atomic_int started_; // ԭ������

    int nextConnId_;
    ConnectionMap connections_; // �������е�����
};