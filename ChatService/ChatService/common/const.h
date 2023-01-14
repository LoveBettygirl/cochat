#ifndef CHATSERVICE_COMMON_CONST_H
#define CHATSERVICE_COMMON_CONST_H

#include <string>

namespace ChatService {

const std::string ONLINE_STATE = "online";
const std::string OFFLINE_STATE = "offline";
const std::string NOT_EXIST_STATE = "not_exist"; // 仅用作缓存，防止缓存穿透

const std::string SAVE_OFFLINE_MSG_TOPIC = "save_offline_msg"; // 异步保存离线消息的topic
const std::string SEND_CHAT_MSG_TOPIC = "send_chat_msg"; // 推送聊天消息的topic

}

#endif