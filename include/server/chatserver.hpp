#ifndef CHATSERVER_H
#define CHATSERBER_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

using namespace muduo;
using namespace muduo::net;

// 聊天服务器的主类
class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,                     //事件循环——Reactor反应堆
               const InetAddress &listenAddr,       //网络中的通信实体的地址信息，服务器地址和端口
               const string &nameArg);              //服务器的名字
    // 启动服务
    void start();

private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &);
    // 上报读写相关信息的回调函数
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);
    TcpServer _server; // 组合muduo库，实现服务器功能的类对象
    EventLoop *_loop;  // 指向事件循环单元的指针
};

#endif