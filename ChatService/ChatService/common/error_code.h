#ifndef CHATSERVICE_COMMON_ERROR_CODE_H
#define CHATSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace ChatService {

enum {
    SUCCESS, 
};

enum {
    USER_LOGGED_IN = 1000, // 账号已经登录，不能重复登录
    CURRENT_USER_NOT_EXIST, // 用户不存在
    INVALID_USER_ID_OR_PWD, // 账号或密码不正确
    REGISTER_FAILED, // 注册失败
    USER_LOGGED_OUT, // 账号已经注销
    USER_LOGOUT_FAILED, // 注销失败
    USER_IS_REGISTERED, // 用户已经注册，不能重复注册
    USER_PWD_IS_EMPTY, // 用户登录或注册输入的密码为空
    USER_NAME_IS_EMPTY, // 用户注册输入的名字为空
    ILLEGAL_USER_HOST, // 用户登录输入的地址不合法
};

enum {
    USER_NOT_IN_FRIEND_RELATION = 2000, // 消息发送者和对方不是好友关系，无法发送消息
    FRIEND_USER_NOT_EXIST, // 好友用户不存在
    FRIEND_RELATION_IS_ADDED, // 已经加好友，不能重复加好友
    FRIEND_RELATION_NOT_EXIST, // 没有好友关系，不能删除好友
    FRIEND_IS_SELF_USER, // 不能对自己添加或删除好友
};

enum {
    GROUP_NOT_EXIST = 3000, // 群组不存在
    GROUP_IS_EXIST, // 群组已存在
    USER_NOT_IN_GROUP, // 消息发送者不在群中，无法发送消息
    USER_IS_IN_GROUP, // 已经在群里了，不能重复加群
    USER_IS_GROUP_CREATOR, // 用户是这个群的创建者，无法退出
    GROUP_NAME_IS_EMPTY, // 创建群输入的名字为空
};

enum {
    FORWARD_CHAT_MSG_FAILED = 4000, // 转发聊天消息失败
    SAVE_OFFLINE_MSG_FAILED, // 存储离线消息失败
    REMOVE_OFFLINE_MSG_FAILED, // 删除离线消息失败
};

enum {
    AES_KEY_LEN_ERROR = 5000, // AES密钥长度不正确
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
    {USER_PWD_IS_EMPTY, "The user pwd is empty"},
    {USER_NAME_IS_EMPTY, "The user name is empty"},
    {ILLEGAL_USER_HOST, "The user host is illegal"},

    {USER_NOT_IN_FRIEND_RELATION, "The user is not in the friend relation"},
    {FRIEND_USER_NOT_EXIST, "The friend user is not exist"},
    {FRIEND_RELATION_IS_ADDED, "The friend relation is added"},
    {FRIEND_RELATION_NOT_EXIST, "The friend relation is not exist"},
    {FRIEND_IS_SELF_USER, "The friend is self user"},

    {GROUP_NOT_EXIST, "The group is not exist"},
    {GROUP_IS_EXIST, "The group is exist"},
    {USER_NOT_IN_GROUP, "The user is not in the group"},
    {USER_IS_IN_GROUP, "The user is in the group"},
    {USER_IS_GROUP_CREATOR, "The user is the group creator"},
    {GROUP_NAME_IS_EMPTY, "The group name is empty"},

    {FORWARD_CHAT_MSG_FAILED, "The chat msg forward failed"},
    {SAVE_OFFLINE_MSG_FAILED, "The offline msg save failed"},
    {REMOVE_OFFLINE_MSG_FAILED, "The offline msg remove failed"},

    {AES_KEY_LEN_ERROR, "The aes key bit length is not 128"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif