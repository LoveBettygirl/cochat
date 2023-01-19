#ifndef GROUPSERVICE_COMMON_CONST_H
#define GROUPSERVICE_COMMON_CONST_H

#include <string>

namespace GroupService {

const std::string CREATOR_ROLE = "creator";
const std::string NORMAL_ROLE = "normal";

const std::string ONLINE_STATE = "online";
const std::string OFFLINE_STATE = "offline";
const std::string NOT_EXIST_STATE = "not_exist"; // 仅用作缓存，防止缓存穿透

const std::string USER_STATE_CACHE_PREFIX = "user_state:";
const std::string USER_HOST_CACHE_PREFIX = "user_host:";

}

#endif