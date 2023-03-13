#ifndef PROXYSERVICE_CHAT_SERVICE_DATA_H
#define PROXYSERVICE_CHAT_SERVICE_DATA_H

#include <string>
#include <corpc/net/custom/custom_data.h>

namespace ProxyService {

class ChatServiceStruct : public corpc::CustomStruct {
public:
    typedef std::shared_ptr<ChatServiceStruct> ptr;
    ChatServiceStruct() {}
    ~ChatServiceStruct() {}

    // int magicBegin;
    uint8_t msgType;
    // int dataLen;
    // std::string protocolData;
    // int magicEnd;
};

}

#endif