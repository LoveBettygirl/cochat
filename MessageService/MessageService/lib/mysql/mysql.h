#ifndef MESSAGESERVICE_LIB_MYSQL_MYSQL_H
#define MESSAGESERVICE_LIB_MYSQL_MYSQL_H

#include <string>
#include <ctime>
#include <memory>
#include <mysql/mysql.h>
#include <yaml-cpp/yaml.h>

namespace MessageService {

class MySQLConfig {
public:
    typedef std::shared_ptr<MySQLConfig> ptr;

    std::string ip;
    uint16_t port;
    std::string user;
    std::string password;
    std::string dbname;
};

/**
 * MySQL数据库操作
*/
class MySQL {
public:
    typedef std::shared_ptr<MySQL> ptr;

    MySQL();
    ~MySQL();

    // 加载数据库信息
    static void loadConf(const YAML::Node &node);

    // 连接数据库
    bool connect();

    // 更新操作
    bool update(const std::string &sql);

    // 查询操作
    MYSQL_RES *query(const std::string &sql);

    // 刷新连接时间
    void refreshAliveTime() { aliveTime_ = clock(); }

    // 返回存活的时间
    clock_t getAliveTime() { return clock() - aliveTime_; }

    // 获取实际连接
    MYSQL *getConnection() const { return mysql_; }

private:
    MYSQL *mysql_; // 和MySQL的连接
    clock_t aliveTime_; // 记录进入空闲状态后的存活时间
};

}

#endif