#ifndef CHATSERVICE_DAO_OFFLINE_MESSAGE_DAO_H
#define CHATSERVICE_DAO_OFFLINE_MESSAGE_DAO_H

#include <string>
#include <vector>
#include "ChatService/lib/mysql/mysql.h"

namespace ChatService {

// 提供离线消息表的操作接口方法
class OfflineMessageDao {
public:

    // 存储用户的离线信息
    bool insert(const std::string &key, int userid, const std::string &msg);

    // 删除用户的所有离线消息
    bool remove(int userid);

    // 查询用户的离线消息
    std::vector<std::string> query(int userid);

private:
    MySQL::ptr mysql_;
};

}

#endif