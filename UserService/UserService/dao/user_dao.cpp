#include <corpc/common/start.h>
#include "UserService/dao/user_dao.h"
#include "UserService/lib/connection_pool.h"

namespace UserService {

UserDao::UserDao()
{
    ConnectionPool<MySQL> *pool = ConnectionPool<MySQL>::getInstance();
    pool->init(corpc::getConfig()->getYamlNode("mysql"));
    mysql_ = pool->getConnection();
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

// 根据用户号码查询用户信息
User UserDao::query(int id)
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
    return User();
}

// 更新用户的状态信息
bool UserDao::updateState(User user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = '%d'", user.getState().c_str(), user.getId());

    return mysql_->update(sql);
}

// 重置用户的状态信息
void UserDao::resetState()
{
    // 1. 组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    mysql_->update(sql);
}

}