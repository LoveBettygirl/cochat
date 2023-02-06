#include "ChatService/common/make_package.h"

namespace ChatService {

std::string makePackage(const std::string &data)
{
    int pkLen = 13 + data.size();
    char *buf = reinterpret_cast<char *>(malloc(pkLen));
    char *temp = buf;

    int magicBegin = htonl(MAGIC_BEGIN);
    memcpy(temp, &magicBegin, sizeof(int32_t));
    temp += sizeof(int32_t);

    int encryptedSizeNet = htonl(data.size() + 1);
    memcpy(temp, &encryptedSizeNet, sizeof(int32_t));
    temp += sizeof(int32_t);

    char pkTypeChar = SERVER_FORWARD_MSG;
    memcpy(temp, &pkTypeChar, sizeof(pkTypeChar));
    temp += sizeof(pkTypeChar);

    memcpy(temp, &*data.begin(), data.size());
    temp += data.size();

    int magicEnd = htonl(MAGIC_END);
    memcpy(temp, &magicEnd, sizeof(int32_t));
    temp += sizeof(int32_t);

    return std::string(buf, buf + pkLen);
}

}