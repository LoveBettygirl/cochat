#ifndef USERSERVICE_COMMON_ERROR_CODE_H
#define USERSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace UserService {

enum {
    SUCCESS, 
    USER_LOGGED_IN, // 账号已经登录，不能重复登录
    CURRENT_USER_NOT_EXIST, // 用户不存在
    INVALID_USER_ID_OR_PWD, // 账号或密码不正确
    REGISTER_FAILED, // 注册失败
    USER_LOGGED_OUT, // 账号已经注销
    USER_LOGOUT_FAILED, // 注销失败
    USER_IS_REGISTERED, // 用户已经注册，不能重复注册
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {USER_LOGGED_IN, "The user is logged in"},
    {CURRENT_USER_NOT_EXIST, "The current user is not exist"},
    {INVALID_USER_ID_OR_PWD, "id or password is invalid"},
    {REGISTER_FAILED, "Register failed"},
    {USER_LOGGED_OUT, "The user is logged out"},
    {USER_LOGOUT_FAILED, "Logout failed"},
    {USER_IS_REGISTERED, "The user is registered"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif