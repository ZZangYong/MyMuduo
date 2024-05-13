#pragma once

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; // 头8个字节，放置这一次要解析的包的长度，防止粘包问题
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    // 可读的数据长度
    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }
    // 可写的数据长度
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    // 防止粘包那一部分的长度
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // 应用只读取了可读缓冲区数据的一部分，就是len，还剩下readerIndex_ += len -> writerIndex_
        }
        else   // len == readableBytes()
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回，给应用使用
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // 确保可写 buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len); // 确保可写的位置足够
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }
    // 可写的地方
    char* beginWrite()
    {
        return begin() + writerIndex_;
    }
    // 常方法，常对象调用，可写的地方
    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int* saveErrno);
private:
    // 获取vector底层数组起始地址
    char* begin()
    {
        // it.operator*()
        // buffer_.begin() 返回迭代器类型
        // *解引用调用了迭代器的重载函数it.operator*()，访问容器底层第0号元素
        // 然后再用&取第0号元素的地址，获取了vector底层数组首元素地址
        return &*buffer_.begin();  // vector底层数组首元素的地址，也就是数组的起始地址
    }
    // 常方法，当有常对象在用的时候调用该方法
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    // 保证底层空间是否充足
    void makeSpace(size_t len)
    {
        /*
            kCheapPrepend | reader | writer
            kCheapPrepend |            len           |
        */
        // writer里可写的+reader和kCheapPrepend之间空闲出来的 < len
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len); // 直接在writerIndex_后面增加len的长度
        }
        else // writer里可写的+reader和kCheapPrepend之间空闲出来的 >= len，就把可读的数据挪到前面
        {
            size_t readalbe = readableBytes();
            std::copy(begin() + readerIndex_, 
                    begin() + writerIndex_,
                    begin() + kCheapPrepend); // 把readerIndex_和writerIndex_之间的数据拷贝到kCheapPrepend后面
            readerIndex_ = kCheapPrepend; 
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_; // 缓冲区
    size_t readerIndex_; // 数据可读位置的下标
    size_t writerIndex_; // 数据可写位置的下标
};