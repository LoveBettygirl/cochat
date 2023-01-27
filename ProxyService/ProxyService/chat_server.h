#ifndef PROXY_SERVICE_CHAT_SERVER_H
#define PROXY_SERVICE_CHAT_SERVER_H

#include <string>
#include <corpc/net/tcp/tcp_connection.h>
#include "ProxyService/common/error_code.h"

namespace ProxyService {

class ChatServer {
public:
    typedef std::shared_ptr<ChatServer> ptr;

    // 初始化聊天服务器对象
    ChatServer();

    // 启动服务
    void start();
private:
    // 上报连接相关信息的回调函数
    void onConnection(const corpc::TcpConnection::ptr &conn);

    // 上报读写事件相关信息的回调函数
    void onMessage(const corpc::TcpConnection::ptr &conn);
};

}

#endif