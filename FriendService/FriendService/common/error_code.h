#ifndef FRIENDSERVICE_COMMON_ERROR_CODE_H
#define FRIENDSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace FriendService {

enum {
    SUCCESS, 
    RELATION_IS_ADDED, // 已经加好友，不能重复加好友
    RELATION_NOT_EXIST, // 没有好友关系，不能删除好友
    ACCOUNT_NOT_EXIST, // 用户不存在
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {RELATION_IS_ADDED, "The relation is added"},
    {RELATION_NOT_EXIST, "The relation is not exist"},
    {ACCOUNT_NOT_EXIST, "The account is not exist"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif