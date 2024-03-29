#ifndef GROUPSERVICE_LIB_REDIS_REDIS_H
#define GROUPSERVICE_LIB_REDIS_REDIS_H

#include <hiredis/hiredis.h>
#include <cstring>
#include <ctime>
#include <string>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace GroupService {

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

    bool setex(const std::string &key, const std::string &value, int time);

    std::string get(const std::string &key);

    bool del(const std::string &key);

    bool expire(const std::string &key, int expireSec);

    int incr(const std::string &key);

    bool flushdb();

    // 刷新连接时间
    void refreshAliveTime() { aliveTime_ = clock(); }

    // 返回存活的时间
    clock_t getAliveTime() { return clock() - aliveTime_; }

    // 获取实际连接
    redisContext *getConnection() const { return cacheContext_; }

private:
    // hiredis同步上下文对象，负责缓存
    redisContext *cacheContext_;
    clock_t aliveTime_; // 记录进入空闲状态后的存活时间
};

// 分布式锁，scoped lock，利用C++的RAII机制自动释放锁
class RedisLock {
public:
    typedef std::shared_ptr<RedisLock> ptr;

    RedisLock(Redis::ptr redis, const std::string &key);
    ~RedisLock();

    bool gotLock() const { return gotLock_; };
    bool unlock();

private:
    Redis::ptr redis_;
    std::string key_;
    bool gotLock_{false};

    bool tryLock();
    bool getLock();
    bool releaseLock();
};

}

#endif