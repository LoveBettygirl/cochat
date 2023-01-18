#ifndef CHATSERVICE_LIB_ROCKETMQ_ROCKETMQ_H
#define CHATSERVICE_LIB_ROCKETMQ_ROCKETMQ_H

#include <memory>
#include <string>
#include <functional>
#include <rocketmq/DefaultMQProducer.h>
#include <rocketmq/DefaultMQPushConsumer.h>
#include <rocketmq/MQMessage.h>
#include <yaml-cpp/yaml.h>
#include <corpc/common/log.h>

namespace ChatService {

class RocketMQProducerConfig {
public:
    typedef std::shared_ptr<RocketMQProducerConfig> ptr;

    std::string ip;
    uint16_t port;
    std::string group;
    int sendMsgTimeout;
    int sendMsgRetries;
};

class RocketMQConsumerConfig {
public:
    typedef std::shared_ptr<RocketMQConsumerConfig> ptr;

    std::string ip;
    uint16_t port;
    std::string group;
    rocketmq::MessageModel messageModel;
    int consumeThreadCount;
};

class RocketMQMsgListener : public rocketmq::MessageListenerConcurrently {
public:
    typedef std::shared_ptr<RocketMQMsgListener> ptr;

    RocketMQMsgListener(std::function<void(const std::string &, const std::string &, const std::string &)> cb) : callback_(cb) {}
    virtual ~RocketMQMsgListener() {}

    virtual rocketmq::ConsumeStatus consumeMessage(const std::vector<rocketmq::MQMessageExt> &msgs) {
        for (int i = 0; i < msgs.size(); i++) {
            USER_LOG_INFO << "recv msg: topic: " << msgs[i].getTopic() << " key: "  << msgs[i].getKeys() << " tag: " << msgs[i].getTags() << " body: "<<  msgs[i].getBody();
            callback_(msgs[i].getKeys(), msgs[i].getTags(), msgs[i].getBody());
        }
        return rocketmq::CONSUME_SUCCESS;
    }

private:
    std::function<void(const std::string &, const std::string &, const std::string &)> callback_;
};

class RocketMQProducer {
public:
    typedef std::shared_ptr<RocketMQProducer> ptr;

    RocketMQProducer();
    ~RocketMQProducer();

public:
    // 加载配置信息
    static void loadConf(const YAML::Node &node);

    void start();

    // 发送消息
    bool send(const std::string &topic, const std::string &data);

    // 发送消息
    bool send(const std::string &topic, const std::string &tag, const std::string &data);

    void shutdown();

private:
    static std::string randomString(std::string::size_type len);

private:
    std::shared_ptr<rocketmq::DefaultMQProducer> producer_;
};

class RocketMQConsumer {
public:
    typedef std::shared_ptr<RocketMQConsumer> ptr;

    RocketMQConsumer(RocketMQMsgListener::ptr listener);
    ~RocketMQConsumer();

public:
    // 加载配置信息
    static void loadConf(const YAML::Node &node);

    void subscribe(const std::string &topic);

    void subscribe(const std::string &topic, const std::string &tag);

    void start();

    void shutdown();

private:
    std::shared_ptr<rocketmq::DefaultMQPushConsumer> consumer_;
    RocketMQMsgListener::ptr listener_;
};

}

#endif
