#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

// ���������
class EchoServer
{
public:
    EchoServer(EventLoop *loop,
            const InetAddress &addr, 
            const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // ע�����ӽ������߶Ͽ��Ļص�
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );
        // ע��ɶ�д�¼��ص�
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        // ���ú��ʵ�loop�߳����� loopthread
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }
private:
    // ���ӽ������߶Ͽ��Ļص�
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str()); // ��ӡ�Զ�IP��ַ�˿ں�
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // �ɶ�д�¼��ص�
    void onMessage(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // д��   EPOLLHUP =�� closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01"); // ����Acceptor,������non-blocking��listenfd,create����socket,bind��socket
    server.start(); // ����listen״̬������loopthread����listenfd�����acceptChannel��Ȼ��ע�ᵽmainLoop��
    loop.loop(); // ����mainLoop�ĵײ�Poller�������¼�

    return 0;
}