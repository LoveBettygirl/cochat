#ifndef PROXY_SERVICE_CHAT_SERVICE_H
#define PROXY_SERVICE_CHAT_SERVICE_H

#include <string>
#include <corpc/net/tcp/tcp_connection.h>
#include "ProxyService/common/error_code.h"
#include "ProxyService/chat_service_dispatcher.h"
#include "ProxyService/chat_service_codec.h"
#include "ProxyService/chat_service_data.h"
#include <corpc/net/custom/custom_service.h>

namespace ProxyService {

class ChatService : public corpc::CustomService {
public:
    typedef std::shared_ptr<ChatService> ptr;

    // 初始化聊天服务器对象
    ChatService();

    std::string getServiceName() override { return "ProxyService"; }

private:
    // 上报连接相关信息的回调函数
    void onConnection(const corpc::TcpConnection::ptr &conn);
};

}

#endif