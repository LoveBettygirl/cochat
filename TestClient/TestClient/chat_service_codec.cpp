#include "TestClient/chat_service_codec.h"
#include "TestClient/chat_service_data.h"
#include "TestClient/common/const.h"
#include <corpc/common/log.h>
#include <corpc/net/byte_util.h>

namespace TestClient {

void ChatServiceCodeC::encode(corpc::TcpBuffer *buf, corpc::AbstractData *data)
{
    ChatServiceStruct *chatServiceStruct = dynamic_cast<ChatServiceStruct *>(data);
    
    int pkLen = 13 + chatServiceStruct->protocolData.size();
    char *buffer = reinterpret_cast<char *>(malloc(pkLen));
    char *temp = buffer;

    int magicBegin = htonl(MAGIC_BEGIN);
    memcpy(temp, &magicBegin, sizeof(int32_t));
    temp += sizeof(int32_t);

    int dataSize = 1 + chatServiceStruct->protocolData.size();
    int dataSizeNet = htonl(dataSize);
    memcpy(temp, &dataSizeNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    char pkTypeChar = chatServiceStruct->msgType;
    memcpy(temp, &pkTypeChar, sizeof(pkTypeChar));
    temp += sizeof(pkTypeChar);

    if (chatServiceStruct->protocolData.size()) {
        memcpy(temp, &*chatServiceStruct->protocolData.begin(), chatServiceStruct->protocolData.size());
        temp += chatServiceStruct->protocolData.size();
    }

    int magicEnd = htonl(MAGIC_END);
    memcpy(temp, &magicEnd, sizeof(int32_t));
    temp += sizeof(int32_t);

    buf->writeToBuffer(buffer, temp - buffer);
}

void ChatServiceCodeC::decode(corpc::TcpBuffer *buf, corpc::AbstractData *data)
{
    LOG_DEBUG << "test custom decode start";
    if (!buf || !data) {
        LOG_ERROR << "decode error! buf or data nullptr";
        return;
    }
    ChatServiceStruct *chatServiceStruct = dynamic_cast<ChatServiceStruct *>(data);
    if (!chatServiceStruct) {
        LOG_ERROR << "not ChatServiceStruct type";
        return;
    }

    std::vector<char> temp = buf->getBufferVector();
    int startIndex = buf->readIndex();
    int endIndex = -1;
    int32_t magic = -1, pkLen = -1;

    bool success = false;

    for (int i = startIndex; i < buf->writeIndex(); i++) {
        if (i + 4 < buf->writeIndex()) {
            magic = corpc::getInt32FromNetByte(&temp[i]);
            if (magic != MAGIC_BEGIN) {
                continue;
            }
            if (i + 8 > buf->writeIndex()) {
                continue;
            }
            pkLen = corpc::getInt32FromNetByte(&temp[i + 4]);
            if (i + 8 + pkLen + 4 > buf->writeIndex()) {
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
        return;
    }

    buf->recycleRead(endIndex + 1 - startIndex);

    pkLen = corpc::getInt32FromNetByte(&temp[startIndex + 4]);

    int pkType = temp[startIndex + 8];
    chatServiceStruct->msgType = pkType;

    if (pkLen > 1) {
        std::string strs(temp.begin() + startIndex + 8 + 1, temp.begin() + startIndex + 8 + pkLen);
        chatServiceStruct->protocolData = strs;
    }

    chatServiceStruct->decodeSucc_ = true;
    data = chatServiceStruct;

    LOG_DEBUG << "test custom decode end";
}

}