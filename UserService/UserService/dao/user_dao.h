#ifndef USERSERVICE_DAO_USER_DAO_H
#define USERSERVICE_DAO_USER_DAO_H

#include <string>
#include "UserService/lib/mysql/mysql.h"
#include "UserService/lib/redis/redis.h"

namespace UserService {

// User表的ORM类
class User {
public:
    User(int id = -1, const std::string &name = "", const std::string &pwd = "", const std::string &state = "offline")
        : id(id), name(name), password(pwd), state(state) {}
    
    void setId(int id) { this->id = id; }
    void setName(const std::string &name) { this->name = name; }
    void setPwd(const std::string &pwd) { this->password = pwd; }
    void setState(const std::string &state) { this->state = state; }

    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getPwd() const { return password; }
    std::string getState() const { return state; }
private:
    int id;
    std::string name;
    std::string password;
    std::string state;
};

// User表的数据操作类
class UserDao {
public:
    UserDao();

    // User表的增加方法
    bool insert(User &user);

    // 根据用户号码查询用户信息，此逻辑不走缓存
    User queryInfo(int id);

    // 查询用户状态信息
    User queryState(int id);

    // 更新用户的状态信息
    bool updateState(User user);

private:
    MySQL::ptr mysql_;
    Redis::ptr redis_;
};

}

#endif