#include "ProxyService/chat_service.h"
#include <functional>
#include <corpc/common/start.h>
#include <corpc/net/byte_util.h>
#include "ProxyService/cypher/aes.h"
#include "ProxyService/common/const.h"

namespace ProxyService {

ChatService::ChatService()
{
    corpc::getServer()->setConnectionCallback(std::bind(&ChatService::onConnection, this, std::placeholders::_1));
    corpc::getServer()->setCustomCodeC(std::make_shared<ChatServiceCodeC>());
    corpc::getServer()->setCustomDispatcher(std::make_shared<ChatServiceDispatcher>());
    corpc::getServer()->setCustomData([]() {  return std::make_shared<ChatServiceStruct>(); });
    // corpc::getServer()->registerService(shared_from_this()); // !!! shared_from_this()不能用在构造函数中，对象还没创建好
}

void ChatService::onConnection(const corpc::TcpConnection::ptr &conn)
{
    // 客户端断开连接
    if (conn->getState() != corpc::Connected) {
        std::shared_ptr<ChatServiceDispatcher> dispatcher = std::dynamic_pointer_cast<ChatServiceDispatcher>(corpc::getServer()->getDispatcher());
        if (dispatcher) {
            dispatcher->clientCloseException(conn);
        }
        conn->shutdownConnection();
        return;
    }
}

}