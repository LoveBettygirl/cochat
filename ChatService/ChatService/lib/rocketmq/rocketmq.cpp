#include "ChatService/lib/rocketmq/rocketmq.h"
#include <corpc/common/log.h>
#include <algorithm>
#include <string>
#include <cctype>
#include "ChatService/utils/id_generator.h"

namespace ChatService {

static RocketMQProducerConfig::ptr gRocketMQProducerConfig;
static int gInitRocketMQProducerConfig = 0;

static RocketMQConsumerConfig::ptr gRocketMQConsumerConfig;
static int gInitRocketMQConsumerConfig = 0;

RocketMQProducer::RocketMQProducer()
{
}

RocketMQProducer::~RocketMQProducer()
{
    if (producer_) {
        producer_->shutdown();
    }
}

void RocketMQProducer::loadConf(const YAML::Node &node)
{
    if (!gRocketMQProducerConfig) {
        gRocketMQProducerConfig = std::make_shared<RocketMQProducerConfig>();
    }
    gRocketMQProducerConfig->ip = node["ip"].as<std::string>();
    gRocketMQProducerConfig->port = node["port"].as<int>();
    gRocketMQProducerConfig->group = node["group"].as<std::string>();
    gRocketMQProducerConfig->sendMsgTimeout = node["send_msg_timeout"].as<int>();
    gRocketMQProducerConfig->sendMsgRetries = node["send_msg_retries"].as<int>();
    gInitRocketMQProducerConfig = 1;
}

void RocketMQProducer::start()
{
    if (!gInitRocketMQProducerConfig) {
        USER_LOG_ERROR << "RocketMQ Producer config is not initialized!";
        return;
    }
    producer_ = std::make_shared<rocketmq::DefaultMQProducer>(gRocketMQProducerConfig->group);
    producer_->setNamesrvAddr(gRocketMQProducerConfig->ip + ":" + std::to_string(gRocketMQProducerConfig->port));
    USER_LOG_INFO << "[RocketMQ] Addr: " << gRocketMQProducerConfig->ip << ":" << gRocketMQProducerConfig->port;
    producer_->setSendMsgTimeout(gRocketMQProducerConfig->sendMsgTimeout);
    producer_->start();
    USER_LOG_INFO << "[RocketMQ] start producer";
}

bool RocketMQProducer::send(const std::string &topic, const std::string &data)
{
    for (int i = 0; i < gRocketMQProducerConfig->sendMsgRetries; i++) {
        std::string tag = "*", key = "Key-" + std::to_string(IDGenerator::getInstance()->getUID(data));
        try {
            rocketmq::MQMessage msg(topic, tag, key, data);
            // 同步生产消息
            rocketmq::SendResult sendResult = producer_->send(msg);
            if (sendResult.getSendStatus() == rocketmq::SEND_OK) {
                USER_LOG_INFO << "[RocketMQ] send msg success: topic: " << topic << " tag: " << tag << " key: " << key << " data: " << data;
                return true;
            }
            USER_LOG_ERROR << "[RocketMQ] send msg failed: topic: " << topic << " tag: " << tag << " key: " << key << " data: " << data;
        }
        catch (rocketmq::MQException &e) {
            USER_LOG_ERROR << "[RocketMQ] send msg failed: topic: " << topic << " tag: " << tag << " key: " << key << " data: " << data << " reason: " << e.what();
        }
    }
    return false;
}

bool RocketMQProducer::send(const std::string &topic, const std::string &tag, const std::string &data)
{
    for (int i = 0; i < gRocketMQProducerConfig->sendMsgRetries; i++) {
        std::string key = "Key-" + std::to_string(IDGenerator::getInstance()->getUID(data));
        try {
            rocketmq::MQMessage msg(topic, tag, key, data);
            // 同步生产消息
            rocketmq::SendResult sendResult = producer_->send(msg);
            if (sendResult.getSendStatus() == rocketmq::SEND_OK) {
                USER_LOG_INFO << "[RocketMQ] send msg success: topic: " << topic << " tag: " << tag << " key: " << key << " data: " << data;
                return true;
            }
            USER_LOG_ERROR << "[RocketMQ] send msg failed: topic: " << topic << " tag: " << tag << " key: " << key << " data: " << data;
        }
        catch (rocketmq::MQException &e) {
            USER_LOG_ERROR << "[RocketMQ] send msg failed: topic: " << topic << " tag: " << tag << " key: " << key << " data: " << data << " reason: " << e.what();
        }
    }
    return false;
}

void RocketMQProducer::shutdown()
{
    producer_->shutdown();
}

RocketMQConsumer::RocketMQConsumer(RocketMQMsgListener::ptr listener) : listener_(listener)
{
}

RocketMQConsumer::~RocketMQConsumer()
{
    if (consumer_) {
        consumer_->shutdown();
    }
}

void RocketMQConsumer::loadConf(const YAML::Node &node)
{
    if (!gRocketMQConsumerConfig) {
        gRocketMQConsumerConfig = std::make_shared<RocketMQConsumerConfig>();
    }
    gRocketMQConsumerConfig->ip = node["ip"].as<std::string>();
    gRocketMQConsumerConfig->port = node["port"].as<int>();
    gRocketMQConsumerConfig->group = node["group"].as<std::string>();
    gRocketMQConsumerConfig->consumeThreadCount = node["consume_thread_count"].as<int>();

    std::string messageModelStr = node["message_model"].as<std::string>();
    std::transform(messageModelStr.begin(), messageModelStr.end(), messageModelStr.begin(), tolower);
    if (messageModelStr == "broadcasting") {
        gRocketMQConsumerConfig->messageModel = rocketmq::BROADCASTING; // 广播模式，同一个消费者组所有消费者都能消费到同一条消息
    }
    else {
        gRocketMQConsumerConfig->messageModel = rocketmq::CLUSTERING; // 集群模式，同一个消费者组只有一个消费者能消费到同一条消息
    }

    gInitRocketMQConsumerConfig = 1;
}

void RocketMQConsumer::subscribe(const std::string &topic)
{
    if (!consumer_) {
        consumer_ = std::make_shared<rocketmq::DefaultMQPushConsumer>(gRocketMQConsumerConfig->group);
    }
    consumer_->subscribe(topic, "*");
}

void RocketMQConsumer::subscribe(const std::string &topic, const std::string &tag)
{
    if (!consumer_) {
        consumer_ = std::make_shared<rocketmq::DefaultMQPushConsumer>(gRocketMQConsumerConfig->group);
    }
    consumer_->subscribe(topic, tag);
}

void RocketMQConsumer::start()
{
    if (!gInitRocketMQConsumerConfig) {
        USER_LOG_ERROR << "RocketMQ Producer config is not initialized!";
        return;
    }
    if (!consumer_) {
        consumer_ = std::make_shared<rocketmq::DefaultMQPushConsumer>(gRocketMQConsumerConfig->group);
    }
    consumer_->setNamesrvAddr(gRocketMQConsumerConfig->ip + ":" + std::to_string(gRocketMQConsumerConfig->port));
    USER_LOG_INFO << "[RocketMQ] Addr: " << gRocketMQConsumerConfig->ip << ":" << gRocketMQConsumerConfig->port;
    consumer_->setConsumeThreadCount(gRocketMQConsumerConfig->consumeThreadCount);
    consumer_->setConsumeFromWhere(rocketmq::CONSUME_FROM_FIRST_OFFSET);
    consumer_->setMessageModel(gRocketMQConsumerConfig->messageModel);
    consumer_->registerMessageListener(listener_.get());
    consumer_->start();
    USER_LOG_INFO << "[RocketMQ] start consumer";
}

void RocketMQConsumer::shutdown()
{
    consumer_->shutdown();
}

}