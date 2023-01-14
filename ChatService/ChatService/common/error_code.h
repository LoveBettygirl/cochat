#ifndef CHATSERVICE_COMMON_ERROR_CODE_H
#define CHATSERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace ChatService {

enum {
    SUCCESS, 
    SEND_CHAT_MSG_FAILED, // 发送聊天消息失败
    SAVE_OFFLINE_MSG_FAILED, // 存储离线消息失败
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {SEND_CHAT_MSG_FAILED, "The chat msg send failed"},
    {SAVE_OFFLINE_MSG_FAILED, "The offline msg save failed"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif