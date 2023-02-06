#include "UserService/lib/redis/redis.h"
#include <corpc/common/log.h>
#include "UserService/common/const.h"

namespace UserService {

static RedisConfig::ptr gRedisConfig;
static int gInitRedisConfig = 0;

Redis::Redis() : cacheContext_(nullptr) {}

Redis::~Redis()
{
    if (cacheContext_ != nullptr) {
        redisFree(cacheContext_);
    }
}

void Redis::loadConf(const YAML::Node &node)
{
    if (!gRedisConfig) {
        gRedisConfig = std::make_shared<RedisConfig>();
    }
    gRedisConfig->ip = node["ip"].as<std::string>();
    gRedisConfig->port = node["port"].as<int>();
    gInitRedisConfig = 1;
}

bool Redis::connect()
{
    if (!gInitRedisConfig) {
        USER_LOG_ERROR << "Redis config is not initialized!";
        return false;
    }

    // 负责缓存的上下文连接
    cacheContext_ = redisConnect(gRedisConfig->ip.c_str(), gRedisConfig->port);
    if (nullptr == cacheContext_) {
        USER_LOG_ERROR << "connect redis failed!";
        return false;
    }

    USER_LOG_INFO << "connect redis-server success!";

    return true;
}

bool Redis::set(const std::string &key, const std::string &value)
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "SET %s %s", key.c_str(), value.c_str());
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis set command failed!";
        return false;
    }
    else if (!(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str, "OK") == 0)) {
        USER_LOG_ERROR << "Redis set command failed!";
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::setex(const std::string &key, const std::string &value, int time)
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "SETEX %s %d %s", key.c_str(), time, value.c_str());
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis set command failed!";
        return false;
    }
    else if (!(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str, "OK") == 0)) {
        USER_LOG_ERROR << "Redis set command failed!";
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

std::string Redis::get(const std::string &key)
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "GET %s", key.c_str());
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis get command failed!";
        return "";
    }
    else if (reply->type != REDIS_REPLY_STRING) {
        USER_LOG_ERROR << "Redis get command failed!";
        freeReplyObject(reply);
        return "";
    }
    std::string result = reply->str;
    freeReplyObject(reply);
    return result;
}

bool Redis::del(const std::string &key)
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "DEL %s", key.c_str());
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis del command failed!";
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::flushdb()
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "FLUSHDB");
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis flushdb command failed!";
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::expire(const std::string &key, int expireSec)
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "EXPIRE %s %d", key.c_str(), expireSec);
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis expire command failed!";
        return false;
    }
    freeReplyObject(reply);
    return true;
}

int Redis::incr(const std::string &key)
{
    redisReply *reply = (redisReply *)redisCommand(cacheContext_, "INCR %s", key.c_str());
    if (nullptr == reply) {
        USER_LOG_ERROR << "Redis incr command failed!";
        return false;
    }
    else if (reply->type != REDIS_REPLY_INTEGER) {
        USER_LOG_ERROR << "Redis incr command failed!";
        freeReplyObject(reply);
        return false;
    }
    int num = reply->integer;
    freeReplyObject(reply);
    return num;
}

RedisLock::RedisLock(Redis::ptr redis, const std::string &key) : redis_(redis), key_(key)
{
    gotLock_ = tryLock();
    if (gotLock_) {
        USER_LOG_INFO << "redis lock success!";
    }
    else {
        USER_LOG_ERROR << "redis lock failed!";
    }
}

RedisLock::~RedisLock()
{
    bool ret = releaseLock();
    if (ret) {
        USER_LOG_INFO << "redis release lock success!";
    }
    else {
        USER_LOG_ERROR << "redis release lock failed!";
    }
}

bool RedisLock::tryLock()
{
    bool ret = getLock();
    if (!ret) {
        for (int i = 0; i < 20; i++) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            ret = getLock();
            if (ret)
                break;
        }
    }
    return redis_->expire(LOCK_PREFIX + key_, 1);
}

bool RedisLock::getLock()
{
    int ret = redis_->incr(LOCK_PREFIX + key_);
    return ret == 1;
}

bool RedisLock::releaseLock()
{
    return redis_->del(LOCK_PREFIX + key_);
}

bool RedisLock::unlock()
{
    return releaseLock();
}

}