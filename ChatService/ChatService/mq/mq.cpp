#include "ChatService/mq/mq.h"
#include "ChatService/common/const.h"
#include "ChatService/dao/offline_message_dao.h"
#include <corpc/common/start.h>

namespace ChatService {

RocketMQProducer::ptr gProducer;
RocketMQConsumer::ptr gOfflineMsgConsumer;
RocketMQMsgListener::ptr gOfflineMsgListener;

void initMqProducer(const YAML::Node &node)
{
    if (!gProducer) {
        gProducer = std::make_shared<RocketMQProducer>();
        gProducer->loadConf(node);
        gProducer->start();
    }
}

void initSaveOfflineMqConsumer(const YAML::Node &node)
{
    if (!gOfflineMsgConsumer) {
        gOfflineMsgListener = std::make_shared<RocketMQMsgListener>([](const std::string &key, const std::string &tag, const std::string &msg) {
            OfflineMessageDao dao;
            dao.insertMessage(key, std::stoi(tag), msg);
        });
        gOfflineMsgConsumer = std::make_shared<RocketMQConsumer>(gOfflineMsgListener);
        gOfflineMsgConsumer->loadConf(node);
        gOfflineMsgConsumer->subscribe(SAVE_OFFLINE_MSG_TOPIC);
        gOfflineMsgConsumer->start();
    }
}

}