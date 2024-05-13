#include "InetAddress.h"

#include <strings.h>
#include <string.h>

// IP��ַת������,��ֹ��ʽת��
// ���õ��ʮ�����ַ�����ʾ��IPv4��ַ���˿ں�ת��Ϊ�������ֽ���������ʾ��IPv4��ַsockaddr_in
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_); // �����ڴ�ռ�����
    addr_.sin_family = AF_INET; // Э���壨IPv4����Э�飩
    // �ѱ����ֽ���һ���޷��Ŷ�������ֵ��ת�������ֽ��򣨴��ģʽ��
    addr_.sin_port = htons(port); 
    // ��������ַipת��IPv4��ַ(�����ֽ��������ֵ)������char*,c_str()��stringת��char
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); 
}

// ��addr_��ȡIP
std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    // �������ֽ���(������ֵ)��ʾ��ip��ַת��Ϊ�ɵ��ʮ���Ƶ�ip��ַ
    // p��n�ֱ�����presentation)�Ͷ�������ֵ��numeric)
    // addr_.sin_addr�������ֽ����������buf�Ǳ����ֽ���
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

// ��ȡIP�Ͷ˿ں�
std::string InetAddress::toIpPort() const
{
    // ��ʽ�� ip:port
    char buf[64] = {0};
    // �������ֽ���(������ֵ)��ʾ��ip��ַת��Ϊ�ɵ��ʮ���Ƶ�ip��ַ
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port); // �����ֽ���ת�ɱ����ֽ���
    sprintf(buf+end, ":%u", port);
    return buf;
}

// ��ȡ�˿ں�
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