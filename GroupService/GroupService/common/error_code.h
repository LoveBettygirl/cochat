#ifndef GROUPSERVICE_COMMON_ERROR_CODE_H
#define GROUPSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace GroupService {

enum {
    SUCCESS, 
    ACCOUNT_IS_IN_GROUP, // 已经在群里了，不能重复加群
    ACCOUNT_NOT_IN_GROUP, // 用户不在这个群，无法退出
    ACCOUNT_IS_GROUP_CREATOR, // 用户是这个群的创建者，无法退出
    GROUP_IS_CREATED, // 此群已经创建过了
    GROUP_IS_NOT_EXIST, // 此群不存在
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {ACCOUNT_IS_IN_GROUP, "The account is in the group"},
    {ACCOUNT_NOT_IN_GROUP, "The account is not in the group"},
    {ACCOUNT_IS_GROUP_CREATOR, "The account is the group creator"},
    {GROUP_IS_CREATED, "The group is created"},
    {GROUP_IS_NOT_EXIST, "The group is not exist"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif