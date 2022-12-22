#ifndef USERSERVICE_COMMON_ERROR_CODE_H
#define USERSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace UserService {

enum {
    SUCCESS, 
    ACCOUNT_LOGGED_IN, // 账号已经登录，不能重复登录
    INVALID_ID_OR_PWD, // 账号或密码不正确
    REGISTER_FAILED, // 注册失败
    ACCOUNT_LOGGED_OUT, // 账号已经注销
    LOGOUT_FAILED, // 注销失败
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {ACCOUNT_LOGGED_IN, "The account is logged in"},
    {INVALID_ID_OR_PWD, "id or password is invalid"},
    {REGISTER_FAILED, "Register failed"},
    {ACCOUNT_LOGGED_OUT, "The account is logged out"},
    {LOGOUT_FAILED, "Logout failed"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif