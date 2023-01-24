#include <corpc/common/start.h>
#include "ChatService/dao/user_dao.h"
#include "ChatService/lib/connection_pool.h"
#include "ChatService/common/const.h"

namespace ChatService {

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
bool UserDao::insertUser(User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, salt, state) values('%s', '%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getSalt().c_str(), user.getState().c_str());

    if (mysql_->update(sql) > 0) {
        // 获取插入成功的用户数据生成的主键id
        user.setId(mysql_insert_id(mysql_->getConnection()));
        return true;
    }
    return false;
}

// 根据用户号码查询用户信息，此逻辑不走缓存
User UserDao::queryUserInfo(int id)
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
            user.setSalt(row[3]);
            user.setState(row[4]);

            mysql_free_result(res);
            return user;
        }
    }
    redis_->set(USER_STATE_CACHE_PREFIX + std::to_string(id), NOT_EXIST_STATE);
    return User(id, "", "", "", NOT_EXIST_STATE);
}

// 查询用户状态信息
User UserDao::queryUserState(int id)
{
    std::string state = redis_->get(USER_STATE_CACHE_PREFIX + std::to_string(id));
    if (!state.empty()) {
        return User(id, "", "", "", state);
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

            redis_->set(USER_STATE_CACHE_PREFIX + std::to_string(user.getId()), user.getState());
            return user;
        }
    }
    redis_->set(USER_STATE_CACHE_PREFIX + std::to_string(id), NOT_EXIST_STATE);
    return User(id, "", "", "", NOT_EXIST_STATE);
}

// 更新用户的状态信息
bool UserDao::updateUserState(const User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = '%d'", user.getState().c_str(), user.getId());

    if (mysql_->update(sql) <= 0) {
        return false;
    }
    // TODO: 加锁、解锁
    redis_->del(USER_STATE_CACHE_PREFIX + std::to_string(user.getId()));
    return true;
}

// 用户登录，缓存用户登录的ProxyServer地址
bool UserDao::saveUserHost(int id, corpc::NetAddress::ptr host)
{
    return redis_->set(USER_HOST_CACHE_PREFIX + std::to_string(id), host->toString());
}

// 查询用户登录的ProxyServer地址
corpc::NetAddress::ptr UserDao::quetyUserHost(int id)
{
    std::string host = redis_->get(USER_HOST_CACHE_PREFIX + std::to_string(id));
    return !host.empty() ? std::make_shared<corpc::IPAddress>(host) : std::make_shared<corpc::IPAddress>(0);
}

bool UserDao::removeUserHost(int id)
{
    return redis_->del(USER_HOST_CACHE_PREFIX + std::to_string(id));
}

}