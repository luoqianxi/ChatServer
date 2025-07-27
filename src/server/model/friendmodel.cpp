#include "friendmodel.hpp"
#include <string>
using namespace std;
#include "db.hpp"

//添加好友关系
void friendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO friend(userid, friendid) VALUES(%d, %d)", userid, friendid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

//返回用户好友列表 friendid  还有好友信息
vector<User> friendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on a.id = b.friendid where b.userid = %d", userid);

    MySQL mysql;
    vector<User> vec;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            //把userid用户的所有离线消息放在vector进行返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;


}