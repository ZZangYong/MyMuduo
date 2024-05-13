#pragma once

#include <vector>
#include <string>
#include <algorithm>

// �����ײ�Ļ��������Ͷ���
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; // ͷ8���ֽڣ�������һ��Ҫ�����İ��ĳ��ȣ���ֹճ������
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    // �ɶ������ݳ���
    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }
    // ��д�����ݳ���
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    // ��ֹճ����һ���ֵĳ���
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // ���ػ������пɶ����ݵ���ʼ��ַ
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // Ӧ��ֻ��ȡ�˿ɶ����������ݵ�һ���֣�����len����ʣ��readerIndex_ += len -> writerIndex_
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

    // ��onMessage�����ϱ���Buffer���ݣ�ת��string���͵����ݷ��أ���Ӧ��ʹ��
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // Ӧ�ÿɶ�ȡ���ݵĳ���
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // ����һ��ѻ������пɶ������ݣ��Ѿ���ȡ����������϶�Ҫ�Ի��������и�λ����
        return result;
    }

    // ȷ����д buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // ���ݺ���
        }
    }

    // ��[data, data+len]�ڴ��ϵ����ݣ���ӵ�writable����������
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len); // ȷ����д��λ���㹻
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }
    // ��д�ĵط�
    char* beginWrite()
    {
        return begin() + writerIndex_;
    }
    // ����������������ã���д�ĵط�
    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // ��fd�϶�ȡ����
    ssize_t readFd(int fd, int* saveErrno);
    // ͨ��fd��������
    ssize_t writeFd(int fd, int* saveErrno);
private:
    // ��ȡvector�ײ�������ʼ��ַ
    char* begin()
    {
        // it.operator*()
        // buffer_.begin() ���ص���������
        // *�����õ����˵����������غ���it.operator*()�����������ײ��0��Ԫ��
        // Ȼ������&ȡ��0��Ԫ�صĵ�ַ����ȡ��vector�ײ�������Ԫ�ص�ַ
        return &*buffer_.begin();  // vector�ײ�������Ԫ�صĵ�ַ��Ҳ�����������ʼ��ַ
    }
    // �����������г��������õ�ʱ����ø÷���
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    // ��֤�ײ�ռ��Ƿ����
    void makeSpace(size_t len)
    {
        /*
            kCheapPrepend | reader | writer
            kCheapPrepend |            len           |
        */
        // writer���д��+reader��kCheapPrepend֮����г����� < len
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len); // ֱ����writerIndex_��������len�ĳ���
        }
        else // writer���д��+reader��kCheapPrepend֮����г����� >= len���Ͱѿɶ�������Ų��ǰ��
        {
            size_t readalbe = readableBytes();
            std::copy(begin() + readerIndex_, 
                    begin() + writerIndex_,
                    begin() + kCheapPrepend); // ��readerIndex_��writerIndex_֮������ݿ�����kCheapPrepend����
            readerIndex_ = kCheapPrepend; 
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_; // ������
    size_t readerIndex_; // ���ݿɶ�λ�õ��±�
    size_t writerIndex_; // ���ݿ�дλ�õ��±�
};