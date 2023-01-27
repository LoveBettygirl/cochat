#include "ProxyService/chat_server.h"
#include "ProxyService/chat_service.h"
#include <functional>
#include <corpc/common/start.h>
#include <corpc/net/byte_util.h>
#include "ProxyService/cypher/aes.h"
#include "ProxyService/common/const.h"

namespace ProxyService {

ChatServer::ChatServer()
{
    corpc::getServer()->setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    corpc::getServer()->setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1));
}

void ChatServer::onConnection(const corpc::TcpConnection::ptr &conn)
{
    // 客户端断开连接
    if (!conn->getState() != corpc::Connected) {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdownConnection();
        return;
    }
    ChatService::instance()->sendPubKeyToClient(conn);
}

void ChatServer::onMessage(const corpc::TcpConnection::ptr &conn)
{
    corpc::TcpBuffer *readBuf = conn->getInBuffer();

    while (readBuf->readAble() > 0) {
        std::vector<char> temp = readBuf->getBufferVector();
        int startIndex = readBuf->readIndex();
        int32_t magic = -1, pkLen = -1;

        if (startIndex + 4 > readBuf->writeIndex()) {
            break;
        }

        magic = corpc::getInt32FromNetByte(&temp[startIndex]);
        if (magic != MAGIC_BEGIN) {
            break;
        }

        if (startIndex + 8 > readBuf->writeIndex()) {
            break;
        }

        pkLen = corpc::getInt32FromNetByte(&temp[startIndex + 4]);
        if (startIndex + 8 + pkLen > readBuf->writeIndex()) {
            break;
        }

        magic = corpc::getInt32FromNetByte(&temp[startIndex + 8 + pkLen]);
        if (magic != MAGIC_END) {
            break;
        }

        readBuf->recycleRead(8 + pkLen + 4);

        std::string strs(temp.begin() + 8, temp.begin() + 8 + pkLen);

        ChatService::instance()->dealWithClient(conn, strs);
    }
}

void ChatServer::start()
{
    corpc::startServer();
}

}