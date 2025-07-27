#include "offinemessagemodel.hpp"
#include "db.hpp"


//存储离线消息
void offineMsgModel::insert(int userid, string msg)
{
    //1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO offinemessage(userid, message) VALUES(%d, '%s')", userid, msg.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

//删除用户的离线消息
void offineMsgModel::remove(int userid)
{
    char sql[1024];
    sprintf(sql, "delete from offinemessage where userid = %d", userid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

//查询用户的离线消息
vector<string> offineMsgModel::query(int userid)
{
    //1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offinemessage where userid = %d", userid);


    //2.获取数据库连接
    MySQL mysql;
    vector<string> vec;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            if(res != nullptr)
            {
                MYSQL_ROW row;
                while((row = mysql_fetch_row(res)) != nullptr)
                {
                    vec.push_back(row[0]);
                }

                mysql_free_result(res);
                return vec;
            }
        }
    }
    return vec;   
}
