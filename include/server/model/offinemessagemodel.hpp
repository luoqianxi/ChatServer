#ifndef OFFINEMESSAGEMODEL_H
#define OFFINEMESSAGEMODEL_H

#include <vector>
#include <string>
using namespace std;

//提供离线消息表的操作接口
class offineMsgModel
{
public:
    //存储用户的离线消息
    void insert(int userid, string msg);

    //删除用户的离线消息
    void remove(int userid);

    //查询用户的离线消息
    vector<string> query(int userid);
};


#endif