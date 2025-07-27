#include "chatserver.hpp"
#include <string>
#include <functional>
#include "json.hpp"
#include "chatservice.hpp"
using namespace std;
using namespace std::placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& nameArg)
    :_server(loop, listenAddr, nameArg), _loop(loop)
{
    //注册链接回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

    //注册读写事件回调
    _server.setMessageCallback(bind(&ChatServer::onMessage,this, _1, _2, _3));

}

void ChatServer::start()
{
    //启动服务器
    _server.start();
}

//上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        //客户端异常退出
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();  
    }
}

//上报读写事件的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                            Buffer* buffer,
                            Timestamp time)
{
    string buf = buffer->retrieveAllAsString();   //获取缓冲区数据

    //解析JSON数据
    json js = json::parse(buf);     
    //目的：完全解耦网络模块的代码和业务模块的代码 
    //onMessage() 收到 JSON 包 { "msgid": 100, "from": 1, "to": 2, "msg": "hi" }
    //通过js["msgid"]获取消息类型 == >> 识别业务类型   

    // | `msgid` | 功能说明      | 业务层调用逻辑示例                                     |
    // | ------- | --------- | --------------------------------------------- |
    // | `1`     | 用户注册      | `chatService->register(conn, js, time);`      |
    // | `2`     | 用户登录      | `chatService->login(conn, js, time);`         |
    // | `3`     | 添加好友      | `chatService->addFriend(conn, js, time);`     |
    // | `100`   | 单聊消息      | `chatService->oneChat(conn, js, time);`       |
    // | `200`   | 创建群组      | `chatService->createGroup(conn, js, time);`   |
    // | `201`   | 加入群组      | `chatService->addGroup(conn, js, time);`      |
    // | `202`   | 群聊消息      | `chatService->groupChat(conn, js, time);`     |
    // | `300`   | 客户端主动退出登录 | `chatService->logout(conn, js, time);`        |
    // | `301`   | 拉取离线消息    | `ChatService->getOfflineMsg(conn, js, time);` |


    //网络层本身不处理具体业务（比如登录、发消息），
    // 但它要根据 msgid 把消息分发到业务层对应的处理函数，
    // 这正是你看到 msgHandler(conn, js, time) 的原因。

    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time); //调用对应的业务处理函数
}