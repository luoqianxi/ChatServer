#include "usermodel.hpp"
#include "db.hpp"
#include <muduo/base/Logging.h>
#include <string>
using namespace std;

//User表的增加方法
bool UserModel::insert(User& user)
{
    //1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO user(name, password, state) VALUES('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());   

    //2.获取数据库连接
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            //3.获取插入成功用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            LOG_INFO << "Insert user success! User ID: " << user.getId();
            return true; //插入成功
        }

    }
    return false; //插入失败
}

//User表的查询方法
User UserModel::query(int id)
{
    //1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);


    //2.获取数据库连接
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();    //如果没查询到返回一个id为-1的用户对象
}

//User表的更新方法
bool UserModel::updatestate(User user)
{
    //1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    //2.获取数据库连接
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false; 
}

void UserModel::resetstate()
{
    char sql[1024] = "update user set state = 'offline' where state ='online'";

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}