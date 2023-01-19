#ifndef GROUPSERVICE_DAO_GROUP_DAO_H
#define GROUPSERVICE_DAO_GROUP_DAO_H

#include "GroupService/model/group.h"
#include <string>
#include <vector>
#include "GroupService/lib/mysql/mysql.h"
#include "GroupService/lib/redis/redis.h"

namespace GroupService {

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

    // 查询用户所在群组信息
    std::vector<Group> queryGroups(int userid);

    // 根据id查询群组
    Group queryGroup(int groupid);

private:
    MySQL::ptr mysql_;
    Redis::ptr redis_;
};

}

#endif