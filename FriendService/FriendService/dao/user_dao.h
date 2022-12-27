#ifndef FRIENDSERVICE_DAO_USER_DAO_H
#define FRIENDSERVICE_DAO_USER_DAO_H

#include <string>
#include "FriendService/lib/mysql/mysql.h"
#include "FriendService/lib/redis/redis.h"
#include "FriendService/dao/user.h"

namespace FriendService {

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