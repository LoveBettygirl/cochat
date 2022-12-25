#ifndef FRIENDSERVICE_LIB_REDIS_REDIS_H
#define FRIENDSERVICE_LIB_REDIS_REDIS_H

#include <hiredis/hiredis.h>
#include <cstring>
#include <string>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace FriendService {

class RedisConfig {
public:
    typedef std::shared_ptr<RedisConfig> ptr;

    std::string ip;
    uint16_t port;
};

/**
 * Redis数据库操作
*/
class Redis {
public:
    typedef std::shared_ptr<Redis> ptr;

    Redis();
    ~Redis();

    // 加载数据库信息
    static void loadConf(const YAML::Node &node);

    // 连接redis服务器 
    bool connect();

    bool set(const std::string &key, const std::string &value);

    std::string get(const std::string &key);

    bool del(const std::string &key);

    bool flushdb();

private:
    // hiredis同步上下文对象，负责缓存
    redisContext *cacheContext_;
};

}

#endif