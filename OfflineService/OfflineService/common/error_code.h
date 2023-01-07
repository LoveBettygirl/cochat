#ifndef OFFLINESERVICE_COMMON_ERROR_CODE_H
#define OFFLINESERVICE_COMMON_ERROR_CODE_H

#include <unordered_map>
#include <string>

namespace OfflineService {

enum {
    SUCCESS, 
    REMOVE_OFFLINE_MSG_FAILED, // 删除离线消息失败
    WRITE_OFFLINE_MSG_FAILED, // 存储离线消息失败
};

const std::unordered_map<int, std::string> code2msg = {
    {SUCCESS, "OK"},
    {REMOVE_OFFLINE_MSG_FAILED, "Remove offline msg failed"},
    {WRITE_OFFLINE_MSG_FAILED, "Write offline msg failed"},
};

const std::string UNKNOWN_ERROR = "Unknown error";

inline std::string getErrorMsg(int code)
{
    auto it = code2msg.find(code);
    return it != code2msg.end() ? it->second : UNKNOWN_ERROR;
}

}

#endif