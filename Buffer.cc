#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据，Poller工作在LT模式，如果fd没有读完的话，底层Poller不断上报，优点：数据不会丢失
 * Buffer缓冲区（堆上内存）是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据（流式数据）最终的大小
 * 
 */ 
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间  65536/1024=64K
    
    struct iovec vec[2]; // 两个缓冲区
    
    // 第一个缓冲区
    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    // 第二个缓冲区（如果第一个缓冲区够就不用管这个，如果不够，数据会自动填到extrabuf里）
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1; // 小于64k就只需要第一个缓冲区，否则需要第二个缓冲区
    const ssize_t n = ::readv(fd, vec, iovcnt); // n是读取的字节数，::readv可以在多个非连续的缓冲区写入同一个fd读出的数据
    if (n < 0) // 出错了
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else // 第二个缓冲区extrabuf里面也要写入数据 
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);  // writerIndex_开始写 n - writable大小的数据
    }

    return n;
}
// 发送数据
ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes()); // 发送数据
    if (n < 0) // 出错了
    {
        *saveErrno = errno;
    }
    return n;
}