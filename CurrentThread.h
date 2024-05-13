#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid; // ����

    void cacheTid();

    // ����������ֻ�ڵ�ǰ�ļ�������
    // ��ȡ�߳�id
    inline int tid()
    {
        // ���t_cachedTid == 0���Ǿ��ǻ�û�л�ȡ����ǰ�߳�id,��˻���һ��
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}