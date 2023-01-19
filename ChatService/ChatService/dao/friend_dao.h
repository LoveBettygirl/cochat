#ifndef CHATSERVICE_DAO_FRIEND_DAO_H
#define CHATSERVICE_DAO_FRIEND_DAO_H

#include <vector>
#include "ChatService/lib/mysql/mysql.h"
#include "ChatService/lib/redis/redis.h"
#include "ChatService/model/user.h"

namespace ChatService {

// 维护好友信息的操作接口方法
class FriendDao {
public:
    FriendDao();

    // 添加好友关系
    bool insertFriend(int userid, int friendid);

    // 删除好友关系
    bool deleteFriend(int userid, int friendid);

    // 判断是否具有好友关系
    bool queryFriend(int userid, int friendid);

    // 返回用户好友列表
    std::vector<User> queryFriendList(int userid);

private:
    MySQL::ptr mysql_;
    Redis::ptr redis_;
};

}

#endif