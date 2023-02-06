#ifndef CHATSERVICE_COMMON_CONST_H
#define CHATSERVICE_COMMON_CONST_H

#include <string>

namespace ChatService {

const std::string ONLINE_STATE = "online";
const std::string OFFLINE_STATE = "offline";
const std::string NOT_EXIST_STATE = "not_exist"; // 仅用作缓存，防止缓存穿透

const std::string SAVE_OFFLINE_MSG_TOPIC = "save_offline_msg"; // 异步保存离线消息的topic

const std::string USER_STATE_CACHE_PREFIX = "user_state:";
const std::string USER_HOST_CACHE_PREFIX = "user_host:";
const std::string LOCK_PREFIX = "lock:";

const int MAGIC_BEGIN = 0xaabbccdd;
const int MAGIC_END = 0xddccbbaa;

enum PackageType {
    CLIENT_CONNECTION, // 客户端连接请求
    SERVER_PUBKEY, // 服务端发送公钥
    CLIENT_KEY, // 客户端发送服务端公钥加密的对称密钥
    CLIENT_MSG, // 跟客户端交互的消息
    SERVER_FORWARD_MSG, // 从其他服务器转发来的消息
    HEARTBEAT, // 心跳消息
};

// 业务类型
enum EnMsgType {
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK, // 登录响应消息
    LOGOUT_MSG, // 注销消息
    LOGOUT_MSG_ACK, // 注销响应消息
    REG_MSG, // 注册消息
    REG_MSG_ACK, // 注册响应消息
    ONE_CHAT_MSG, // 聊天消息
    ONE_CHAT_MSG_ACK, // 聊天消息响应
    ADD_FRIEND_MSG, // 添加好友消息
    ADD_FRIEND_MSG_ACK, // 添加好友响应消息
    DEL_FRIEND_MSG, // 删除好友消息
    DEL_FRIEND_MSG_ACK, // 删除好友响应消息
    CREATE_GROUP_MSG, // 创建群组
    CREATE_GROUP_MSG_ACK, // 创建群组响应消息
    ADD_GROUP_MSG, // 加入群组
    ADD_GROUP_MSG_ACK, // 加入群组响应消息
    QUIT_GROUP_MSG, // 退出群组
    QUIT_GROUP_MSG_ACK, // 退出群组响应消息
    GROUP_CHAT_MSG, // 群聊天
    GROUP_CHAT_MSG_ACK, // 群聊天响应
    FORWARDED_MSG, // 转发过来的消息
    GET_USER_INFO_MSG, // 获取当前用户信息消息
    GET_USER_INFO_MSG_ACK, // 获取当前用户信息消息响应
};

}

#endif