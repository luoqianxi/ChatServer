#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <muduo/net/EventLoop.h>
using namespace std;
#include <signal.h>
#include <muduo/base/Logging.h>

//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    LOG_INFO << "capture the SIGINT, will reset state\n";
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //当程序收到 SIGINT 信号时（通常是用户按 Ctrl+C），就调用你定义的 resetHandler 函数。
    signal(SIGINT, resetHandler);
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();  // 启动事件循环
    return 0;
}