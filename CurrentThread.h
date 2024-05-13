#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid; // 声明

    void cacheTid();

    // 内联函数，只在当前文件起作用
    // 获取线程id
    inline int tid()
    {
        // 如果t_cachedTid == 0，那就是还没有获取过当前线程id,因此缓存一下
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}