#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装Socket地址类型
class InetAddress
{
public:
    // IP地址转换函数,禁止隐式转换
    // 将用点分十进制字符串表示的IPv4地址及端口号转化为用网络字节序整数表示的IPv4地址sockaddr_in
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    // 直接传过来sockaddr_in
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string toIp() const; // 获取IP
    std::string toIpPort() const; // 获取IP和端口号
    uint16_t toPort() const; // 获取端口号

    // 获取成员变量addr_
    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_; // IPv4专用socket地址结构体
};