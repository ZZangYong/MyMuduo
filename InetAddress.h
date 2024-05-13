#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// ��װSocket��ַ����
class InetAddress
{
public:
    // IP��ַת������,��ֹ��ʽת��
    // ���õ��ʮ�����ַ�����ʾ��IPv4��ַ���˿ں�ת��Ϊ�������ֽ���������ʾ��IPv4��ַsockaddr_in
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    // ֱ�Ӵ�����sockaddr_in
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string toIp() const; // ��ȡIP
    std::string toIpPort() const; // ��ȡIP�Ͷ˿ں�
    uint16_t toPort() const; // ��ȡ�˿ں�

    // ��ȡ��Ա����addr_
    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_; // IPv4ר��socket��ַ�ṹ��
};