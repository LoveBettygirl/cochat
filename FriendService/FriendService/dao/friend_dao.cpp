#include <corpc/common/start.h>
#include "FriendService/dao/friend_dao.h"
#include "FriendService/lib/connection_pool.h"

namespace FriendService {

FriendDao::FriendDao()
{
    ConnectionPool<MySQL> *mysqlPool = ConnectionPool<MySQL>::getInstance();
    mysqlPool->init(corpc::getConfig()->getYamlNode("mysql"));
    mysql_ = mysqlPool->getConnection();

    ConnectionPool<Redis> *redisPool = ConnectionPool<Redis>::getInstance();
    redisPool->init(corpc::getConfig()->getYamlNode("redis"));
    redis_ = redisPool->getConnection();
}

// 添加好友关系
bool FriendDao::insertFriend(int userid, int friendid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    return mysql_->update(sql);
}

// 删除好友关系
bool FriendDao::deleteFriend(int userid, int friendid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from friend where userid = %d and friendid = %d", userid, friendid);

    return mysql_->update(sql);
}

// 返回用户好友列表
std::vector<User> FriendDao::queryFriendList(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where userid=%d", userid);

    std::vector<User> vec;

    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        // 把userid用户的所有离线消息放入vec中返回
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.emplace_back(user);
            redis_->set("state:" + std::to_string(user.getId()), user.getState());
        }
        mysql_free_result(res);
        return vec;
    }

    return vec;
}

}