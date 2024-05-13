#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <unistd.h>
#include <atomic>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    /// �߳�����ָ������
    typedef std::shared_ptr<Thread> ptr;
    
    /**
     * @brief ���캯��
     * @param[in] func �߳�ִ�к���
     * @param[in] name �߳�����
     */
    explicit Thread(ThreadFunc func, const std::string& name = std::string()); // ��ֹ��ʽת��
    
    /**
     * @brief ��������
     */
    ~Thread();

    /**
     * @brief �����߳�
     */
    void start();

    /**
     * @brief �ȴ��߳�ִ�����
     */
    void join();

    // /**
    //  * @brief ��ȡ��ǰ���߳�ָ��
    //  */
    // static Thread *GetThis();

    /**
     * @brief ��ȡ�߳�����״̬
     */
    bool started() const { return started_; }

    /**
     * @brief ��ȡ�߳�ID
     */
    pid_t tid() const { return tid_; }

    /**
     * @brief ��ȡ�߳�����
     */
    const std::string& name() const { return name_; }

    /**
     * @brief ��ȡ�����̸߳���
     */
    static int numCreated() { return numCreated_; }

private:
    /**
     * @brief �����߳�Ĭ������
     */
    void setDefaultName();

    bool started_;
    bool joined_; // ��ǰ�̵߳ȴ������߳��������ˣ���ǰ�߳��ټ�����������
    std::shared_ptr<std::thread> thread_; // ����ֱ��ʹ��std::thread thread_; ��Ϊ�����̴߳����˾ͻ���������������Ҫ�ܿ����߳�������ʱ��
    pid_t tid_; // �̵߳�tid
    ThreadFunc func_; // �洢�̺߳���
    std::string name_; // �߳�����
    static std::atomic_int numCreated_; // ��¼�����̵߳ĸ�������̬����
};