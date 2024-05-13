#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => ��һ�����û����ӣ�ͨ��accept�����õ�connfd
 * =�� TcpConnection ���ûص� =�� Channel =�� Poller =�� Channel�Ļص�����
 * 
 */ 
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, 
                const std::string &name, 
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; } // �����Ƿ����ӳɹ�

    // ��������
    void send(const std::string &buf);
    // �ر�����
    void shutdown();
    // ���ûص�
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    // ���ӽ���
    void connectEstablished();
    // ��������
    void connectDestroyed();
private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting}; // ����״̬��δ���ӣ������У������ӣ��Ͽ�������
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // ������Բ���baseLoop�� ��ΪTcpConnection������subLoop��������
    const std::string name_; // ��������
    std::atomic_int state_;
    bool reading_;

    // �����Acceptor����   Acceptor=��mainLoop    TcpConenction=��subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_; // ��ǰ����ip��ַ�˿ں�
    const InetAddress peerAddr_; // �Զ�����ip��ַ�˿ں�

    ConnectionCallback connectionCallback_; // ��������ʱ�Ļص�
    MessageCallback messageCallback_; // �ж�д��Ϣʱ�Ļص�
    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ��������Ժ�Ļص�
    HighWaterMarkCallback highWaterMarkCallback_; // ��ˮλ�Ļص������ͷ��ͽ��շ��������������һ�£��������̫�����ͣ���ͣ�
    CloseCallback closeCallback_;
    size_t highWaterMark_; // ˮλ��ʶ������ˮλ���Ǹ�ˮλ

    Buffer inputBuffer_;  // �������ݵĻ�����
    Buffer outputBuffer_; // �������ݵĻ�����
};