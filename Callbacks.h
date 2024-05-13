#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>; // 指向TcpConnection的智能指针
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>; // 有新连接的回调
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>; // 消息发送完成以后的回调
using MessageCallback = std::function<void (const TcpConnectionPtr&,
                                        Buffer*,
                                        Timestamp)>; // 有读写消息时的回调
using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>; // 高水位回调