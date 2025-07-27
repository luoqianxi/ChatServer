#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "json.hpp"
#include <vector>
#include <map>
#include <iostream>
using namespace std;
using namespace muduo;


//获取单例对象的静态接口函数
ChatService* ChatService::instance()
{
    static ChatService instance;
    return &instance;
}

//注册消息以及对于的handler回调操作
ChatService::ChatService()
{
    // //用户事件的相关处理回调
    // _msgHandlerMap[LOGIN_MSG] = std::bind(&ChatService::login, this, _1, _2, _3);
    // _msgHandlerMap[LOGOUT_MSG] = std::bind(&ChatService::loginout, this, _1, _2, _3);
    // _msgHandlerMap[REG_MSG] = std::bind(&ChatService::registerUser, this, _1, _2, _3);
    // _msgHandlerMap[ONE_CHAT_MSG] = std::bind(&ChatService::oneChat, this, _1, _2, _3);
    // _msgHandlerMap[ADD_FRIEND_MSG] = std::bind(&ChatService::addFriend, this, _1, _2, _3);

    // //群组事务管理相关事件处理回调
    // _msgHandlerMap[CREATE_GROUP_MSG] = std::bind(&ChatService::createGroup, this, _1, _2, _3);
    // _msgHandlerMap[ADD_GROUP_MSG] = std::bind(&ChatService::addGroup, this, _1, _2, _3);
    // _msgHandlerMap[GROUP_CHAT_MSG] = std::bind(&ChatService::groupChat, this, _1, _2, _3);

    //用户事件的相关处理回调
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::registerUser, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    //群组事务管理相关事件处理回调
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    //连接redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }

}

MegHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，mesgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr& conn,
                            json& js,
                            Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " is invalid!";
        };
    }

    return _msgHandlerMap[msgid]; //返回对应的消息处理函数
}


//处理登录业务
void ChatService::login(const TcpConnectionPtr& conn,
                       json& js,
                       Timestamp time)
{
    LOG_DEBUG << "do login service!";

    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if(user.getId() != -1 && pwd == user.getPassword())
    {
        if(user.getState() == "online")
        {
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            response["id"] = user.getId();
            response["name"] = user.getName();
            conn->send(response.dump());
        }
        else
        {
            {
                //登录成功，记录用户连接信息，在多线程环境下，要加锁，锁的力度要小
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            
            //id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            //登录成功，更新用户状态信息 state offline =>online
            //数据库的并发操作由mysql server保证
            user.setState("Online");
            _userModel.updatestate(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            //查询该用户是否有离线消息
            vector<string> vec = _offineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取该用户离线消息后，删除用户的离线消息
                _offineMsgModel.remove(id);
            }

            //查询该用户的好友信息并返回
            vector<User> userVecs = _friendModel.query(id);
            if(!userVecs.empty())
            {
                vector<string> userVec;
                for(User &user : userVecs)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();

                    userVec.push_back(js.dump());
                }
                response["friends"] = userVec;
            }

            //查询用户的群组消息, 包括群名，群id，群信息，群成员
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty())
            {
                //group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for(Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    vector<string> userV;
                    for(GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }

                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] =  groupV;
            }
            
            conn->send(response.dump());
        }
        
    }
    else
    {
        //该用户不存在或密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalied!";
        conn->send(response.dump());
    }

}

//处理注册业务 
void ChatService::registerUser(const TcpConnectionPtr& conn,
                              json& js,
                              Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    
    if(state)     //自动创建、默认为offline
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}


//处理注销用户
void ChatService::loginout(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }  
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户状态信息
    User user(userid, "", "", "offline");
    _userModel.updatestate(user);
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    LOG_INFO << "clientCloseException called";
    User user;
    {
        lock_guard<mutex> lock(_connMutex);  

        //遍历状态为online的map表的信息
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if(it->second == conn)
            {
                //找出断开连接对应的用户ID，
                //写入 user 对象中，供后续使用（例如更新数据库用户状态为 offline）。
                user.setId(it->first);  
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    if(user.getId() != -1)
    {
        //更新用户的状态信息 online->offline
        user.setState("offline");
        _userModel.updatestate(user);
    }
}

//处理服务器端异常退出
void ChatService::reset()
{
    //把onlie状态的所有用户，设置成offline
    _userModel.resetstate();
}


//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);

        //服务器推送消息给在线用户
        if(it != _userConnMap.end())        //即toid此时是在线，即和客户端能够进行tcp连接
        {
            //toid在线，转发消息  服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    //查询to是否在线
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid ,js.dump());
        return;
    }

    //toid用户不在线，存储离线消息,相当于要发送的用户不在线，存储这个不在线的消息
    _offineMsgModel.insert(toid, js.dump());
}

//添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid,friendid);
}

//创建群组业务， 用户想创建一个群聊，且有一个群主
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group))          //创建一个群聊（存入数据库表 allgroup）
    {
        _groupModel.addGroup(userid, group.getId(), "creator");    //将当前用户以“创建者”身份添加到 groupuser 表中
    }
}

//加入群组业务   让一个用户加入某个已有的群组，角色是普通成员（"normal"）
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

//群组聊天业务    这一步会查询该群内除了自己之外的所有用户。  
//在线成员立刻收到群消息；
//离线成员将在下次登录时收到离线消息；
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    //查询除了该用户外其他在该群组里面的用户
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end())
        {
            //转发群消息,如果该用户在线
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                //存储离线消息，如果该用户不在线
                _offineMsgModel.insert(id, js.dump());
            }
        }
    }
}


// 用户 A 连接到服务器 A
// ↓
// 用户 A 向好友 B 发消息
// ↓
// 服务器 A publish(用户B的ID, 消息内容)
// ↓
// Redis 负责把消息发给订阅了“用户B频道”的其他服务器
// ↓
// 服务器 B 收到 Redis 的回调（调用 handleRedisSubscribeMessage）
// ↓
// 服务器 B 查找用户 B 是否在线
// ↓
// 若在线，转发消息；若不在线，保存为离线消息


//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offineMsgModel.insert(userid, msg);
}


