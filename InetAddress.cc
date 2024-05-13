#include "InetAddress.h"

#include <strings.h>
#include <string.h>

// IP地址转换函数,禁止隐式转换
// 将用点分十进制字符串表示的IPv4地址及端口号转化为用网络字节序整数表示的IPv4地址sockaddr_in
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_); // 将该内存空间清零
    addr_.sin_family = AF_INET; // 协议族（IPv4网络协议）
    // 把本地字节序（一个无符号短整型数值）转成网络字节序（大端模式）
    addr_.sin_port = htons(port); 
    // 将主机地址ip转成IPv4地址(网络字节序二进制值)，传入char*,c_str()将string转成char
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); 
}

// 从addr_获取IP
std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    // 将网络字节序(二进制值)表示的ip地址转化为成点分十进制的ip地址
    // p和n分别代表表达（presentation)和二进制数值（numeric)
    // addr_.sin_addr还网络字节序的整数，buf是本地字节序
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

// 获取IP和端口号
std::string InetAddress::toIpPort() const
{
    // 格式： ip:port
    char buf[64] = {0};
    // 将网络字节序(二进制值)表示的ip地址转化为成点分十进制的ip地址
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port); // 网络字节序转成本地字节序
    sprintf(buf+end, ":%u", port);
    return buf;
}

// 获取端口号
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// #include <iostream>
// int main()
// {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
//     return 0;
// }