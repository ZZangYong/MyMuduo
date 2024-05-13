#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>; // ָ��TcpConnection������ָ��
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>; // �������ӵĻص�
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>; // ��Ϣ��������Ժ�Ļص�
using MessageCallback = std::function<void (const TcpConnectionPtr&,
                                        Buffer*,
                                        Timestamp)>; // �ж�д��Ϣʱ�Ļص�
using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>; // ��ˮλ�ص�