#ifndef USER_H
#define USER_H
#include <string>
using namespace std;

//User类的ORM类
class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state; // 默认状态为离线
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPassword(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() const { return this->id; }
    string getName() const { return this->name; }
    string getPassword() const { return this->password; }
    string getState() const { return this->state; }

protected:
    int id;
    string name;
    string password;
    string state; // 用户状态
};



#endif