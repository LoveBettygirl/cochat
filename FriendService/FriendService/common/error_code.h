#ifndef FRIENDSERVICE_COMMON_ERROR_CODE_H
#define FRIENDSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace FriendService {

enum {
    SUCCESS, 
    FRIEND_RELATION_IS_ADDED, // 已经加好友，不能重复加好友
    FRIEND_RELATION_NOT_EXIST, // 没有好友关系，不能删除好友
    CURRENT_USER_NOT_EXIST, // 当前用户不存在
    FRIEND_USER_NOT_EXIST, // 好友用户不存在
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {FRIEND_RELATION_IS_ADDED, "The friend relation is added"},
    {FRIEND_RELATION_NOT_EXIST, "The friend relation is not exist"},
    {CURRENT_USER_NOT_EXIST, "The current user is not exist"},
    {FRIEND_USER_NOT_EXIST, "The friend user is not exist"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif