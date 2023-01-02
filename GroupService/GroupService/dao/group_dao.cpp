#include <corpc/common/start.h>
#include "GroupService/dao/group_dao.h"
#include "GroupService/lib/connection_pool.h"

namespace GroupService {

GroupDao::GroupDao()
{
    ConnectionPool<MySQL> *mysqlPool = ConnectionPool<MySQL>::getInstance();
    mysqlPool->init(corpc::getConfig()->getYamlNode("mysql"));
    mysql_ = mysqlPool->getConnection();

    ConnectionPool<Redis> *redisPool = ConnectionPool<Redis>::getInstance();
    redisPool->init(corpc::getConfig()->getYamlNode("redis"));
    redis_ = redisPool->getConnection();
}

// 创建群组
bool GroupDao::createGroup(Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());

    if (mysql_->update(sql)) {
        group.setId(mysql_insert_id(mysql_->getConnection()));
        return true;
    }

    return false;
}

// 加入群组
bool GroupDao::addGroup(int userid, int groupid, const std::string &role)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')",
            groupid, userid, role.c_str());

    return mysql_->update(sql);
}

// 根据id查询群组
Group GroupDao::queryGroup(int groupid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from allgroup where id = %d", groupid); // 查询组的信息

    Group group;

    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr) {
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            mysql_free_result(res);
        }
    }

    // 根据groupid查询组内成员
    sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid=%d", group.getId());

    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            GroupUser user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            user.setRole(row[3]);
            group.getUsers().push_back(user);
            redis_->set("state:" + std::to_string(user.getId()), user.getState());
        }
        mysql_free_result(res);
    }
    return group;
}

// 查询用户所在群组信息
std::vector<Group> GroupDao::queryGroups(int userid)
{
    /**
     * 1. 先根据userid在groupuser表中查询出该用户所属的群组信息
     * 2. 在根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
         groupuser b on a.id = b.groupid where b.userid = %d", userid);

    std::vector<Group> groupVec;

    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        MYSQL_ROW row;
        // 查出userid所有的群组信息
        while ((row = mysql_fetch_row(res)) != nullptr) {
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }

    // 查询群组的用户信息
    for (Group &group : groupVec) {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid = %d", group.getId());

        MYSQL_RES *res = mysql_->query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
                redis_->set("state:" + std::to_string(user.getId()), user.getState());
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 根据指定的groupid查询群组用户列表
std::vector<int> GroupDao::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and \
        exists(select 1 from groupuser where groupid = %d and userid = %d)", groupid, groupid, userid);

    std::vector<int> idVec;
    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            idVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return idVec;
}

}