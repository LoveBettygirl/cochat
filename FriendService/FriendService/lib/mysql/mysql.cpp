#include "FriendService/lib/mysql/mysql.h"
#include <corpc/common/log.h>

namespace FriendService {

static MySQLConfig::ptr gMySQLConfig;
static int gInitMySQLConfig = 0;

MySQL::MySQL()
{
    mysql_ = mysql_init(nullptr);
}

MySQL::~MySQL()
{
    if (!mysql_) {
        mysql_close(mysql_);
    }
}

void MySQL::loadConf(const YAML::Node &node)
{
    if (!gMySQLConfig) {
        gMySQLConfig = std::make_shared<MySQLConfig>();
    }
    gMySQLConfig->ip = node["ip"].as<std::string>();
    gMySQLConfig->port = node["port"].as<int>();
    gMySQLConfig->user = node["user"].as<std::string>();
    gMySQLConfig->password = node["password"].as<std::string>();
    gMySQLConfig->dbname = node["dbname"].as<std::string>();
    gInitMySQLConfig = 1;
}

bool MySQL::connect()
{
    if (!gInitMySQLConfig) {
        USER_LOG_ERROR << "MySQL config is not initialized!";
        return false;
    }
    if (!mysql_real_connect(mysql_, gMySQLConfig->ip.c_str(), gMySQLConfig->user.c_str(), gMySQLConfig->password.c_str(), gMySQLConfig->dbname.c_str(), gMySQLConfig->port, nullptr, 0)) {
        USER_LOG_ERROR << "connect MySQL failed!";
        return false;
    }
    USER_LOG_INFO << "connect MySQL server success!";
    return true;
}

int MySQL::update(const std::string &sql)
{
    if (mysql_query(mysql_, sql.c_str())) {
        USER_LOG_ERROR << "MySQL update error: " << sql;
        return -1;
    }
    return mysql_affected_rows(mysql_);
}

MYSQL_RES *MySQL::query(const std::string &sql)
{
    if (mysql_query(mysql_, sql.c_str())) {
        USER_LOG_ERROR << "MySQL query error: " << sql;
        return nullptr;
    }
    return mysql_use_result(mysql_);
}

}
