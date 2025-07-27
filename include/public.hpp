#ifndef PUBLIC_H
#define PUBLIC_H

/*
server 和 client 之间的公共头文件
包含了网络通信中使用的常量、数据结构和函数声明等
用于定义消息类型、用户信息等
这样可以确保 server 和 client 之间的通信协议一致
同时也便于维护和扩展
例如：定义消息类型的枚举、用户信息结构体等
这样可以避免在 server 和 client 中重复定义相同的内容
同时也可以在需要修改时只需修改一个地方即可
这有助于提高代码的可读性和可维护性
*/
enum EnMegType
{
    LOGIN_MSG = 1,          //登录消息
    LOGIN_MSG_ACK,              //登录响应
    REG_MSG,                //注册消息
    REG_MSG_ACK,                //注册响应
    ONE_CHAT_MSG = 100,     //单聊消息
    ADD_FRIEND_MSG,         //添加好友消息
    CREATE_GROUP_MSG = 200, //创建群组消息
    ADD_GROUP_MSG,          //加入群组消息
    GROUP_CHAT_MSG = 201,   //群聊消息
    LOGOUT_MSG = 300,       //客户端主动退出登录
    GET_OFFLINEMSG_MSG = 301,//拉取离线消息
};


#endif