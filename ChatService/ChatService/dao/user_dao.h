#ifndef CHATSERVICE_DAO_USER_DAO_H
#define CHATSERVICE_DAO_USER_DAO_H

#include <string>
#include "ChatService/lib/mysql/mysql.h"
#include "ChatService/lib/redis/redis.h"
#include "ChatService/dao/user.h"

namespace ChatService {

// User表的数据操作类
class UserDao {
public:
    UserDao();

    // User表的增加方法
    bool insertUser(User &user);

    // 根据用户号码查询用户信息，此逻辑不走缓存
    User queryUserInfo(int id);

    // 查询用户状态信息
    User queryUserState(int id);

    // 更新用户的状态信息
    bool updateUserState(User user);

private:
    MySQL::ptr mysql_;
    Redis::ptr redis_;
};

}

#endif