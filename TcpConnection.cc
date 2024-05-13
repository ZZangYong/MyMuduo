#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

// static ��֤�����ʱ�����ֲ���ͻ
// ��֤loop�ǿ�
static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, 
                const std::string &nameArg, 
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting) // ��ʼ״̬����������
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr) // ������ַ
    , peerAddr_(peerAddr) // �Զ˵�ַ
    , highWaterMark_(64*1024*1024) // 64M
{
    // �����channel������Ӧ�Ļص�������poller��channel֪ͨ����Ȥ���¼������ˣ�channel��ص���Ӧ�Ĳ�������
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true); // �����������
}


TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected) // �ѽ�������
    {
        if (loop_->isInLoopThread()) // ��ǰloop�ڶ�Ӧ�߳���
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else // �����Ѷ�Ӧ�߳�
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}

/**
 * ��������  
 * Ӧ��д�Ŀ죬���ں˷�������������Ҫ�Ѵ���������д�뻺����������������ˮλ�ص�
 */ 
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0; // �˴η����ѷ������ݳ���
    size_t remaining = len; // �˴η���δ�������ݳ���
    bool faultError = false;

    // ֮ǰ���ù���connection��shutdown�������ٽ��з�����
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // ��ʾchannel_��һ�ο�ʼд���ݣ�֮ǰ��д�¼�������Ȥ�������һ�����û�д���������
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) // ���ͳɹ�
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // ��Ȼ����������ȫ��������ɣ��Ͳ����ٸ�channel����epollout�¼���
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else // nwrote < 0 ������
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // EWOULDBLOCK�Ƿ�����û�����ݵķ��أ������ľ�����Ĵ�����
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    faultError = true;
                }
            }
        }
    }

    // ˵����ǰ��һ��write����û�а�����ȫ�����ͳ�ȥ��ʣ���������Ҫ���浽���������У�Ȼ���channel
    // ע��epollout�¼���poller����tcp�ķ��ͻ������пռ䣬��֪ͨ��Ӧ��sock-channel������writeCallback_�ص�����
    // Ҳ���ǵ���TcpConnection::handleWrite�������ѷ��ͻ������е�����ȫ���������
    if (!faultError && remaining > 0) 
    {
        // Ŀǰ���ͻ�����ʣ��Ĵ��������ݵĳ���
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ // ���ͻ�����ʣ��Ĵ��������ݵĳ���+�˴η���δ�������ݵĳ��ȴ���ˮλ��
            && oldLen < highWaterMark_ // ���ͻ�����ʣ��Ĵ��������ݵĳ���С��ˮλ�ߣ�����Ǵ���˵��֮ǰ�Ѿ�ִ�й���ˮλ�ص��ˣ�
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen+remaining)
            );
        }
        outputBuffer_.append((char*)data + nwrote, remaining); // ��δ���͵�����д�ط��ͻ�������(char*)data + nwroteָδ���͵����ݵ��׵�ַ
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // ����һ��Ҫע��channel��д�¼�������poller�����channel֪ͨepollout
        }
    }
}

// �ر�����
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this)
        );
    }
}
// ���ͻ�������ȫ�����ݷ�����Źر�д��
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // ˵��outputBuffer�е������Ѿ�ȫ���������
    {
        socket_->shutdownWrite(); // �ر�д��
    }
}

// ���ӽ���
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); // ����ָ���TcpConnection�����ָ�룬��ֹ�䱻�ֶ��Ƴ�ʱ����ִ�лص�
    channel_->enableReading(); // ��pollerע��channel��epollin�¼�

    // �����ӽ�����ִ�лص����û��󶨵Ļص���
    connectionCallback_(shared_from_this());
}

// ��������
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // ��channel�����и���Ȥ���¼�����poller��del��
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // ��channel��poller��ɾ����
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) // ���ݶ�ȡ����
    {
        // �ѽ������ӵ��û����пɶ��¼������ˣ������û�����Ļص�����onMessage
        // shared_from_this()��ȡ��ǰTCP���������ָ��
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else // ������
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
// �������ݲ���
void TcpConnection::handleWrite()
{
    if (channel_->isWriting()) // ͨ����д
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno); // ͨ��buffer��������
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // ����loop_��Ӧ��thread�̣߳�ִ�лص�
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop(); // �ڵ�ǰ����Loop���TCPConnectionɾ����
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else // ͨ������д
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

// poller => channel::closeCallback => TcpConnection::handleClose
// poller֪ͨchannel�رգ�channel����closeCallback�ص�����TCPConnectionԤ�õ�handleClose
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll(); // ��channel�����и���Ȥ���¼�����poller��del��

    TcpConnectionPtr connPtr(shared_from_this());
    // û���ж��Ƿ��лص������˻ص���������̫��
    connectionCallback_(connPtr); // ִ�����ӹرյĻص�
    closeCallback_(connPtr); // �ر����ӵĻص�  ִ�е���TcpServer::removeConnection�ص�����
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}