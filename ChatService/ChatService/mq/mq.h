#ifndef CHATSERVICE_MQ_MQ_H
#define CHATSERVICE_MQ_MQ_H

#include "ChatService/lib/rocketmq/rocketmq.h"
#include <yaml-cpp/yaml.h>

namespace ChatService {

void initMqProducer(const YAML::Node &node);
void initSaveOfflineMqConsumer(const YAML::Node &node);

}

#endif