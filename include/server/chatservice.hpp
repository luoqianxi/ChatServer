//业务层
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "offinemessagemodel.hpp"
#include "redis.hpp"
#include "groupmodel.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <muduo/base/Timestamp.h>
using muduo::Timestamp;
#include "json.hpp"
using json = nlohmann::json;
#include <mutex>
using namespace std;
using namespace muduo::net;
using MegHandler = std::function<void(const TcpConnectionPtr&,
                                        json&,
                                      Timestamp)>;

//聊天服务器业务类
//单例模式：一个类只有一个对象，对全局的处理
class ChatService
{
public:

    static ChatService* instance();


    //处理登录业务
    void login(const TcpConnectionPtr& conn,
                    json& js,
                     Timestamp time);

    //处理注册业务
    void registerUser(const TcpConnectionPtr& conn,
                      json& js,
                      Timestamp time);

    //一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn,
                      json& js,
                      Timestamp time);


    //获取消息对于的处理器，即发生了处理业务
    MegHandler getHandler(int msgid);

    //处理注销业务
    void loginout(const TcpConnectionPtr& conn,
                      json& js,
                      Timestamp time);

    //处理客户端异常退出 Ctrl+] → quit（Telnet） 客户端断开 TCP 连接
    void clientCloseException(const TcpConnectionPtr &conn);
    

    //服务器异常，业务重置方法  Ctrl+C（在服务端终端）  服务端进程接收 SIGINT 并退出
    void reset();

    //添加好友业务 msgid id friendid
    void addFriend(const TcpConnectionPtr& conn,
                      json& js,
                      Timestamp time);

    //创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //用户加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void handleRedisSubscribeMessage(int, string);


private:
    
    ChatService();

    //存储消息id及其对应事件处理方法
    unordered_map<int, MegHandler> _msgHandlerMap; //消息处理函数映射表

    //存储在线的用户的通信连接 online
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    //在多线程环境下，通信要用锁来保证线程安全
    mutex _connMutex;

    //数据操作类对象
    //ChatService 是程序的大脑，
    //它理解用户发来的请求，并调用 UserModel 去数据库里查数据、存数据。
    UserModel _userModel;
    offineMsgModel _offineMsgModel;
    friendModel _friendModel;
    groupModel _groupModel;

    //redis操作对象
    Redis _redis;
};



#endif // CHATSERVER_H