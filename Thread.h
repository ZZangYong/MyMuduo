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

    /// 线程智能指针类型
    typedef std::shared_ptr<Thread> ptr;
    
    /**
     * @brief 构造函数
     * @param[in] func 线程执行函数
     * @param[in] name 线程名称
     */
    explicit Thread(ThreadFunc func, const std::string& name = std::string()); // 禁止隐式转换
    
    /**
     * @brief 析构函数
     */
    ~Thread();

    /**
     * @brief 启动线程
     */
    void start();

    /**
     * @brief 等待线程执行完成
     */
    void join();

    // /**
    //  * @brief 获取当前的线程指针
    //  */
    // static Thread *GetThis();

    /**
     * @brief 获取线程启动状态
     */
    bool started() const { return started_; }

    /**
     * @brief 获取线程ID
     */
    pid_t tid() const { return tid_; }

    /**
     * @brief 获取线程名称
     */
    const std::string& name() const { return name_; }

    /**
     * @brief 获取产生线程个数
     */
    static int numCreated() { return numCreated_; }

private:
    /**
     * @brief 设置线程默认名称
     */
    void setDefaultName();

    bool started_;
    bool joined_; // 当前线程等待其他线程运行完了，当前线程再继续往下运行
    std::shared_ptr<std::thread> thread_; // 不能直接使用std::thread thread_; 因为这样线程创建了就会马上启动，我们要能控制线程启动的时机
    pid_t tid_; // 线程的tid
    ThreadFunc func_; // 存储线程函数
    std::string name_; // 线程名字
    static std::atomic_int numCreated_; // 记录产生线程的个数，静态变量
};