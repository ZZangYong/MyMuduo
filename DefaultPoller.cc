#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

// DefaultPoller.cc是一个公共的源文件,因此添加依赖关系没问题
// 但是Poller是最上层的抽象基类，派生类可以引用它，但它最好不要引用派生类
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    // 如果提前设置了环境变量名字MUDUO_USE_POLL就执行
    if (::getenv("MUDUO_USE_POLL")) // 获取环境变量
    {
        return nullptr; // 生成poll的实例
    }
    else // 默认没有设置环境变量名字MUDUO_USE_POLL，因此默认使用EPOLL
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
}