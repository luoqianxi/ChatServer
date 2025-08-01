#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

//记录当前系统登录的用户消息
User g_currentUser;

//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;

//记录当前登录用户的群组列表消息
vector<Group> g_currentUserGroupList;

//显示当前登录成功用户的基本信息
void showCurrentUserData();

//接收线程
void readTaskHandler(int clientfd);

//获取系统时间（聊天信息需要添加时间消息）
string getCurrentTime();

//主聊天页面程序
void mainMenu(int clientfd);



//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if(argc < 3)         //解析命令
    {
        cerr << "command invalid example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    //解析通过命令行参数传递ip和port
    char *ip = argv[1];            //127.0.0.1
    uint16_t port = atoi(argv[2]);      //6000


    //进行和服务器的连接
    //创建client端的socket AF_INET：使用 IPv4 协议族；, SOCK_STREAM：使用 TCP 流式套接字； 0：让系统自动选择对应协议（对于 SOCK_STREAM 来说就是 TCP）。
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);

    if(-1 == clientfd)
    {
        cerr << "socket creat error" << endl;
        exit(-1);
    }

    //// 配置服务器地址
    //填写client需要连接的server信息ip + port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //client和server进行连接
    if(-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connet server error" << endl;
        close(clientfd);
        exit(-1);
    }

    //main线程用于接收用户输入、负责发送数据
    for(;;)
    {
        //显示首页面菜单、登录、注册、退出
        cout << "===========================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "============================" << endl;
        cout << "choice:" ;
        int choice = 0;
        cin >> choice;
        cin.get();   //读取缓冲区残留的回车
        
        switch(choice)
        {
            case 1:     //login业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "userid:";
                cin >> id;
                cin.get();   //读掉缓冲区残留的回车

                cout << "userpassword:";
                cin.getline(pwd, 50);
                
                json js;
                js["msgid"] = LOGIN_MSG;
                js["password"] = pwd;
                js["id"] = id;

                string request = js.dump();
                
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1)
                {
                    cerr << "send login msg errno:" << request << endl;
                }
                else
                {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if(-1 == len)
                    {
                        cerr << "recv login response errno" << endl;
                    }
                    else
                    {
                        json responsejs = json::parse(buffer);
                        if(0 != responsejs["errno"].get<int>())  //登录失败
                        {
                            cerr << responsejs["errmsg"] << endl;
                        }
                        else  //登录成功
                        {
                            //记录当前用户的id和name
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);

                            //记录当前用户的好友列表信息
                            if(responsejs.contains("friends"))
                            {
                                // 初始化
                                g_currentUserFriendList.clear();

                                vector<string> vec = responsejs["friends"];

                                for(string &str : vec)
                                {
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            //记录当前用户的群组列表信息 包括群名，群id，群信息，群成员
                            if(responsejs.contains("groups"))
                            {
                                // 初始化
                                g_currentUserGroupList.clear();

                                vector<string> vec1 = responsejs["groups"];

                                for(string &groupstr : vec1)
                                {
                                    json grpjs = json::parse(groupstr);
                                    Group group;
                                    group.setId(grpjs["id"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);

                                    vector<string> vec2 = grpjs["users"];
                                    for(string &userstr : vec2)
                                    {
                                        GroupUser user;
                                        json js = json::parse(userstr);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);

                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            //显示登录用户的基本信息
                            showCurrentUserData();

                            //显示当前用户的离线消息 个人聊天消息或者群组消息
                            if(responsejs.contains("offlinemsg"))
                            {
                                vector<string> vec = responsejs["offlinemsg"];
                                for(string &str : vec)
                                {
                                    json js = json::parse(str);
                                    int msgtype = js["msgid"].get<int>();
                                    if(ONE_CHAT_MSG == msgtype)
                                    {
                                        cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                                            << "said: " << js["msg"].get<string>() << endl;
                                    }
                                    else
                                    {
                                        cout << "群消息[ " << js["groupid"] << "]:" << js["time"].get<string>() << " [" 
                                            << js["id"] << "said: " << js["msg"].get<string>() << endl;
                                    }
                                }
                            }

                            //登录成功，启动接收线程负责接收数据；

                            //| 线程                      | 作用                    |
                            //| ----------------------- | --------------------- |
                            //| 主线程                     | 处理用户输入（命令解析、发送消息）     |
                            //| 子线程 `readTaskHandler()` | **不断接收服务端消息**，并在控制台打印 |

                            std::thread readTask(readTaskHandler, clientfd);     //pthread_create
                            readTask.detach();    //pthread_detach

                            //进入聊天主菜单页面
                            mainMenu(clientfd);
                        }
                    }
                }
            }
            break;

            case 2:  //register业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username: ";
                cin.getline(name, 50);
                cout << "userpassword:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1)
                {
                    cerr << "send reg msg error" << request << endl;
                }
                else
                {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    cout << buffer << endl;
                    if(-1 == len)
                    {
                        cerr << "recv reg response error" << endl;
                    }
                    else 
                    {
                        json responsejs = json::parse(buffer);
                        if(0 != responsejs["errno"].get<int>())   //注册失败，这是注册业务规定的
                        {
                            cerr << name << "is already exitst, register error! " << endl;
                        }
                        else
                        {
                            cout << name << "  register success, userid is" << responsejs["id"]
                                << ", do not forget it!" << endl;
                        }
                    }
                }
            }
            break;

            case 3: //quit业务
            {
                close(clientfd);
                exit(0);
            
            }
            default:
                cerr << "invalid input!" << endl;
                break;
        }
        
    }
    return 0;
}

void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

//接收线程
void readTaskHandler(int clientfd) {
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if(len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        //接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                << "said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if(GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[ " << js["groupid"] << "]:" << js["time"].get<string>() << " [" 
                << js["id"] << "said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}


//其中一般int传入clientfd, string为
// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);


// 系统支持的客户端命令列表  用户命令说明文档

// 这个哈希表（unordered_map）的作用是：
// 保存所有客户端支持的指令。
// 每条命令对应一个字符串，说明命令的用途和使用格式。
// 主要用于 help() 函数中，向用户显示帮助信息。
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理    当传入前面的值，就对应执行后面的函数
// 使用 function<void(int, string)> 是为了统一接口，所有命令函数都接受两个参数：
// int：连接用的 socket fd
// string：命令参数
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void mainMenu(int clientfd){
    help();          //为了告诉客户端用户系所支持的所有命令及注释
    
    char buffer[1024] = {0};
    for(;;)
    {
        cin.getline(buffer, 1024);     //用户输入命令
        string commandbuf(buffer);
        string command; //存储命令
        int idx = commandbuf.find(":"); 

        if(idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);     //只获取前面的命令
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        //commandbuf.substr(idx + 1, commandbuf.size() - idx) 为除命令以外后面的字符串
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));    //调用命令处理方法
    }
}

//"help" command handler
void help(int, string)
{
    cout << "show command list >>>"  << endl;
    for(auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

//"addfriend" command handler
void addfriend(int clientfd, string str) {
    int friendid = atoi(str.c_str());   
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

//"chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "chat command invalid" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName(); 
    js["to"] = friendid; 
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

// "groupchat" command handler   groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}

// "loginout" command handler
void loginout(int clientfd, string)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error -> " << buffer << endl;
    }
//     else
//     {
//         isMainMenuRunning = false;
//     }   
}


// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return date;
}

