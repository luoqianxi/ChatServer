#ifndef CHATSERVER_H
#define CHATSERVER_H
#include <string>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const string& nameArg);
    
    //启动服务器
    void start();

private:
    //上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr&);

    //上报读写事件的回调函数
    void onMessage(const TcpConnectionPtr&,
                   Buffer* buf,
                   Timestamp time);

    EventLoop* _loop;   
    TcpServer _server;   //组合的muduo库
};

#endif