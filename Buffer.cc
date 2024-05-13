#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * ��fd�϶�ȡ���ݣ�Poller������LTģʽ�����fdû�ж���Ļ����ײ�Poller�����ϱ����ŵ㣺���ݲ��ᶪʧ
 * Buffer�������������ڴ棩���д�С�ģ� ���Ǵ�fd�϶����ݵ�ʱ��ȴ��֪��tcp���ݣ���ʽ���ݣ����յĴ�С
 * 
 */ 
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0}; // ջ�ϵ��ڴ�ռ�  65536/1024=64K
    
    struct iovec vec[2]; // ����������
    
    // ��һ��������
    const size_t writable = writableBytes(); // ����Buffer�ײ㻺����ʣ��Ŀ�д�ռ��С
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    // �ڶ����������������һ�����������Ͳ��ù������������������ݻ��Զ��extrabuf�
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1; // С��64k��ֻ��Ҫ��һ����������������Ҫ�ڶ���������
    const ssize_t n = ::readv(fd, vec, iovcnt); // n�Ƕ�ȡ���ֽ�����::readv�����ڶ���������Ļ�����д��ͬһ��fd����������
    if (n < 0) // ������
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffer�Ŀ�д�������Ѿ����洢��������������
    {
        writerIndex_ += n;
    }
    else // �ڶ���������extrabuf����ҲҪд������ 
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);  // writerIndex_��ʼд n - writable��С������
    }

    return n;
}
// ��������
ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes()); // ��������
    if (n < 0) // ������
    {
        *saveErrno = errno;
    }
    return n;
}