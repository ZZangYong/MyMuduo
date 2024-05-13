#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop; // ���͵�ǰ������
// EventLoop�൱�ڶ�·�¼��ַ�����һ���߳�һ����one loop per thread��
// һ��EventLoop������һ��Poller��һ��Channel��һ���߳̿��Լ�����·Channel����·fd��
// Channel�������fd�������Ȥ��event�����շ������¼�
// ��Щ�¼�����Ҫ��Poller��ע�ᣬ�������¼���Poller��Channel֪ͨ
// Channel�õ���Ӧfd��֪ͨ�󣬵���Ԥ�ƵĻص�����
/*
Channel���Ϊͨ������װ��sockfd�������Ȥ��event,��EPOLLIN��EPOLLOUT�¼�
������poller���صľ����¼���
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>; // �¼��Ļص�
    using ReadEventCallback = std::function<void(Timestamp)>; // ֻ���¼��Ļص�

    Channel(EventLoop *Loop, int fd); // ��װ��sockfd�������Ȥ��event
    ~Channel();

    // fd�õ�poller֪ͨ�Ժ󣬴����¼���
    void handleEvent(Timestamp receiveTime);

    // ���ûص���������
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb);} // ����ֵת����ֵ
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    // ��ֹ��channel���ֶ�remove����channel����ִ�лص�����
    // ʹ����ָ��tie_����channel�Ƿ��ֶ�remove��
    void tie(const std::shared_ptr<void>&);
    // ���س�Ա������ֵ
    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revent(int revt) {revents_ = revt;}
    
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    // ��channel�����и���Ȥ���¼�����poller��del��
    void disableAll() { events_ = kNoneEvent; update(); }

    // ����fd��ǰ���¼�״̬
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // ��ҵ��ǿ��أ����߼�����Բ�ǿ
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    void set_revents(int revt) { revents_ = revt; }

    // һ���߳�һ��Loop : one loop per thread
    EventLoop* ownerLoop() {return loop_;}
    
    // ɾ��Channel
    void remove();
private:
    void update(); // ��poller�������fd��Ӧ���¼�epoll_ctl()
    void handleEventWithGuard(Timestamp receiveTime); // �ܱ����Ĵ����¼�

    static const int kNoneEvent; // û�ж��κ��¼�����Ȥ
    static const int kReadEvent; // �Զ��¼�����Ȥ
    static const int kWriteEvent; // ��д�¼�����Ȥ

    EventLoop *loop_; // �¼�ѭ��
    const int fd_;    // fd,Poller�����Ķ���
    int events_; // ע��fd����Ȥ���¼�
    int revents_; // poller���ص�ʵ�ʾ��巢�����¼����ͣ�epoll����poll��
    int index_; // Poller�õ�

    std::weak_ptr<void> tie_; // �۲�һ��ǿ����ָ�룬���Լ���Ҳ������shared_from_this()�����ָ�������shared_ptr
    bool tied_; // �Ƿ��ǿ����ָ��shared_ptr

    // ��ΪChannelͨ�������ܹ���֪fd���շ����ľ�����¼�revents,������������þ����¼��Ļص�����
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};