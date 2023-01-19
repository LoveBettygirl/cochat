#ifndef GROUPSERVICE_COMMON_ERROR_CODE_H
#define GROUPSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace GroupService {

enum {
    SUCCESS, 
    CURRENT_USER_NOT_EXIST, // 当前用户不存在
    USER_IS_IN_GROUP, // 已经在群里了，不能重复加群
    USER_NOT_IN_GROUP, // 用户不在这个群，无法退出
    USER_IS_GROUP_CREATOR, // 用户是这个群的创建者，无法退出
    GROUP_IS_EXIST, // 此群已存在，无法创建
    GROUP_NOT_EXIST, // 此群不存在
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {CURRENT_USER_NOT_EXIST, "The current user is not exist"},
    {USER_IS_IN_GROUP, "The user is in the group"},
    {USER_NOT_IN_GROUP, "The user is not in the group"},
    {USER_IS_GROUP_CREATOR, "The user is the group creator"},
    {GROUP_IS_EXIST, "The group is exist"},
    {GROUP_NOT_EXIST, "The group is not exist"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif