#ifndef CHATSERVICE_COMMON_ERROR_CODE_H
#define CHATSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace ChatService {

enum {
    SUCCESS, 
    GROUP_NOT_EXIST, // 群组不存在
    USER_NOT_IN_GROUP, // 消息发送者不在群中，无法发送消息
    USER_NOT_IN_FRIEND_RELATION, // 消息发送者和对方不是好友关系，无法发送消息
    FORWARD_CHAT_MSG_FAILED, // 转发聊天消息失败
    SAVE_OFFLINE_MSG_FAILED, // 存储离线消息失败
    CURRENT_USER_NOT_EXIST, // 当前用户不存在
    FRIEND_USER_NOT_EXIST, // 好友用户不存在
    REMOVE_OFFLINE_MSG_FAILED, // 删除离线消息失败
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {GROUP_NOT_EXIST, "The group is not exist"},
    {USER_NOT_IN_GROUP, "The user is not in the group"},
    {USER_NOT_IN_FRIEND_RELATION, "The user is not in the friend relation"},
    {FORWARD_CHAT_MSG_FAILED, "The chat msg forward failed"},
    {SAVE_OFFLINE_MSG_FAILED, "The offline msg save failed"},
    {CURRENT_USER_NOT_EXIST, "The current user is not exist"},
    {FRIEND_USER_NOT_EXIST, "The friend user is not exist"},
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