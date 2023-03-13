#ifndef CHATSERVICE_PROTOCOL_CHAT_SERVICE_CODEC_H
#define CHATSERVICE_PROTOCOL_CHAT_SERVICE_CODEC_H

#include <corpc/net/custom/custom_codec.h>
#include <corpc/net/tcp/tcp_buffer.h>

namespace ChatService {

class ChatServiceCodeC : public corpc::CustomCodeC {
public:
    typedef std::shared_ptr<ChatServiceCodeC> ptr;
    ChatServiceCodeC() {}
    ~ChatServiceCodeC() {}

    void encode(corpc::TcpBuffer *buf, corpc::AbstractData *data) override;
    void decode(corpc::TcpBuffer *buf, corpc::AbstractData *data) override;
};

}

#endif