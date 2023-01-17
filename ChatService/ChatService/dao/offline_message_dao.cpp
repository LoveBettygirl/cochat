#include <corpc/common/start.h>
#include "ChatService/dao/offline_message_dao.h"
#include "ChatService/lib/connection_pool.h"

namespace ChatService {

OfflineMessageDao::OfflineMessageDao()
{
    ConnectionPool<MySQL> *mysqlPool = ConnectionPool<MySQL>::getInstance();
    mysqlPool->init(corpc::getConfig()->getYamlNode("mysql"));
    mysql_ = mysqlPool->getConnection();
}

// 存储用户的离线信息
bool OfflineMessageDao::insert(const std::string &key, int userid, const std::string &msg)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values('%s', %d, '%s')", key.c_str(), userid, msg.c_str());

    return mysql_->update(sql);
}

// 删除用户的所有离线消息
bool OfflineMessageDao::remove(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    return mysql_->update(sql);
}

// 查询用户的离线消息
std::vector<std::string> OfflineMessageDao::query(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    std::vector<std::string> vec;
    MYSQL_RES *res = mysql_->query(sql);
    if (res != nullptr) {
        // 把userid用户的所有离线消息放入vec中返回
        MYSQL_ROW row;
        // 把userid用户的所有离线消息放入vec中返回
        while ((row = mysql_fetch_row(res)) != nullptr) {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}

}