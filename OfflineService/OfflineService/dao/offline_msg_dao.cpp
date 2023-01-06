#include <corpc/common/start.h>
#include "OfflineService/dao/offline_msg_dao.h"
#include "OfflineService/lib/connection_pool.h"

namespace OfflineService {

OfflineMsgDao::OfflineMsgDao()
{
    ConnectionPool<MySQL> *mysqlPool = ConnectionPool<MySQL>::getInstance();
    mysqlPool->init(corpc::getConfig()->getYamlNode("mysql"));
    mysql_ = mysqlPool->getConnection();
}

bool OfflineMsgDao::insert(int userid, const std::string &msg)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());

    return mysql_->update(sql);
}

bool OfflineMsgDao::remove(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    return mysql_->update(sql);
}

std::vector<std::string> OfflineMsgDao::query(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

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