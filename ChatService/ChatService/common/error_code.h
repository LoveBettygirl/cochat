#ifndef CHATSERVICE_COMMON_ERROR_CODE_H
#define CHATSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace ChatService {

enum {
    SUCCESS, 
    USER_NOT_IN_GROUP, // 消息发送者不在群中，无法发送消息
    USER_NOT_IN_FRIEND_RELATION, // 消息发送者和对方不是好友关系，无法发送消息
    SEND_CHAT_MSG_FAILED, // 发送聊天消息失败
    SAVE_OFFLINE_MSG_FAILED, // 存储离线消息失败
    ACCOUNT_NOT_EXIST, // 用户不存在
    REMOVE_OFFLINE_MSG_FAILED, // 删除离线消息失败
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {USER_NOT_IN_GROUP, "The user is not in the group"},
    {USER_NOT_IN_FRIEND_RELATION, "The user is not in the friend relation"},
    {SEND_CHAT_MSG_FAILED, "The chat msg send failed"},
    {SAVE_OFFLINE_MSG_FAILED, "The offline msg save failed"},
    {ACCOUNT_NOT_EXIST, "The account is not exist"},
    {REMOVE_OFFLINE_MSG_FAILED, "The offline msg remove failed"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif