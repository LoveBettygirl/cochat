#ifndef CHATSERVICE_COMMON_CONST_H
#define CHATSERVICE_COMMON_CONST_H

#include <string>

namespace ChatService {

const std::string ONLINE_STATE = "online";
const std::string OFFLINE_STATE = "offline";
const std::string NOT_EXIST_STATE = "not_exist"; // 仅用作缓存，防止缓存穿透

}

#endif