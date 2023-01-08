#ifndef FRIENDSERVICE_COMMON_CONST_H
#define FRIENDSERVICE_COMMON_CONST_H

#include <string>

namespace FriendService {

const std::string ONLINE_STATE = "online";
const std::string OFFLINE_STATE = "offline";
const std::string NOT_EXIST_STATE = "not_exist"; // 仅用作缓存，防止缓存穿透

}

#endif