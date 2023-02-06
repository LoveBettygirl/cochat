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
    ChatService::instance(); // 调用构造函数
}

void ChatServer::onConnection(const corpc::TcpConnection::ptr &conn)
{
    // 客户端断开连接
    if (conn->getState() != corpc::Connected) {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdownConnection();
        return;
    }
    // ChatService::instance()->sendPubKeyToClient(conn);
}

void ChatServer::onMessage(const corpc::TcpConnection::ptr &conn)
{
    corpc::TcpBuffer *readBuf = conn->getInBuffer();

    while (readBuf->readAble() > 0) {
        std::vector<char> temp = readBuf->getBufferVector();
        int startIndex = readBuf->readIndex();
        int endIndex = -1;
        int32_t magic = -1, pkLen = -1;

        bool success = false;

        for (int i = startIndex; i < readBuf->writeIndex(); i++) {
            if (i + 4 < readBuf->writeIndex()) {
                magic = corpc::getInt32FromNetByte(&temp[i]);
                if (magic != MAGIC_BEGIN) {
                    continue;
                }
                if (i + 8 > readBuf->writeIndex()) {
                    continue;
                }
                pkLen = corpc::getInt32FromNetByte(&temp[i + 4]);
                if (i + 8 + pkLen + 4 > readBuf->writeIndex()) {
                    continue;
                }
                magic = corpc::getInt32FromNetByte(&temp[i + 8 + pkLen]);
                if (magic == MAGIC_END) {
                    success = true;
                    startIndex = i;
                    endIndex = i + 12 + pkLen - 1;
                    break;
                }
            }
        }

        if (!success) {
            break;
        }

        readBuf->recycleRead(endIndex + 1 - startIndex);

        pkLen = corpc::getInt32FromNetByte(&temp[startIndex + 4]);

        int pkType = temp[startIndex + 8];

        std::string strs(temp.begin() + startIndex + 8 + 1, temp.begin() + startIndex + 8 + pkLen);

        ChatService::instance()->dealWithClient(conn, pkType, strs);
    }
}

void ChatServer::start()
{
    corpc::startServer();
}

}