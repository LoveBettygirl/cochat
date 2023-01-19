#ifndef CHATSERVICE_DAO_GROUP_DAO_H
#define CHATSERVICE_DAO_GROUP_DAO_H

#include "ChatService/model/group.h"
#include <string>
#include <vector>
#include "ChatService/lib/mysql/mysql.h"
#include "ChatService/lib/redis/redis.h"

namespace ChatService {

// 维护群组信息的操作接口方法
class GroupDao {
public:
    GroupDao();

    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    bool addGroup(int userid, int groupid, const std::string &role);

    // 查询用户所在组的角色
    std::string GroupDao::queryGroupUserRole(int userid, int groupid);

    // 退出群组
    bool quitGroup(int userid, int groupid);

    // 查询用户所有群组信息
    std::vector<Group> queryGroups(int userid);

    // 根据id查询群组
    Group queryGroup(int groupid);

private:
    MySQL::ptr mysql_;
    Redis::ptr redis_;
};

}

#endif