#include "db.hpp"
#include <muduo/base/Logging.h>

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL()
{
    if (_conn != nullptr)
    mysql_close(_conn);
}

bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
    password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        //C和c++代码默认的编码字符是ASCII，如果数据库的编码是GBK或者UTF-8，需要设置一下
        //否则会出现中文乱码问题
        mysql_query(_conn, "set names gbk");


        LOG_INFO << "MySQL connect success!";
    }
    else
    {
        LOG_INFO << "MySQL connect failed! Error: ";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
        << sql << "更新失败!" << mysql_error(_conn);
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
        << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}