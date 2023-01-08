#include <corpc/common/start.h>
#include "FriendService/dao/user_dao.h"
#include "FriendService/lib/connection_pool.h"
#include "FriendService/common/const.h"

namespace FriendService {

UserDao::UserDao()
{
    ConnectionPool<MySQL> *mysqlPool = ConnectionPool<MySQL>::getInstance();
    mysqlPool->init(corpc::getConfig()->getYamlNode("mysql"));
    mysql_ = mysqlPool->getConnection();

    ConnectionPool<Redis> *redisPool = ConnectionPool<Redis>::getInstance();
    redisPool->init(corpc::getConfig()->getYamlNode("redis"));
    redis_ = redisPool->getConnection();
}

// User表的增加方法
bool UserDao::insert(User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    if (mysql_->update(sql)) {
        // 获取插入成功的用户数据生成的主键id
        user.setId(mysql_insert_id(mysql_->getConnection()));
        return true;
    }
    return false;
}

// 根据用户号码查询用户信息，此逻辑不走缓存
User UserDao::queryInfo(int id)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr) {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);

            mysql_free_result(res);
            return user;
        }
    }
    redis_->set("state:" + std::to_string(id), NOT_EXIST_STATE);
    return User(id, "", "", NOT_EXIST_STATE);
}

// 查询用户状态信息
User UserDao::queryState(int id)
{
    std::string state = redis_->get("state:" + std::to_string(id));
    if (!state.empty()) {
        return User(id, "", "", state);
    }

    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select id, state from user where id = %d", id);

    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr) {
            User user;
            user.setId(atoi(row[0]));
            user.setState(row[1]);

            mysql_free_result(res);

            redis_->set("state:" + std::to_string(user.getId()), user.getState());
            return user;
        }
    }
    redis_->set("state:" + std::to_string(id), NOT_EXIST_STATE);
    return User(id, "", "", NOT_EXIST_STATE);
}

// 更新用户的状态信息
bool UserDao::updateState(User user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = '%d'", user.getState().c_str(), user.getId());

    if (!mysql_->update(sql)) {
        return false;
    }
    // TODO: 加锁、解锁
    redis_->del("state:" + std::to_string(user.getId()));
    return true;
}

}