#ifndef PROXYSERVICE_CYPHER_AES_H
#define PROXYSERVICE_CYPHER_AES_H

#include <memory>
#include <string>
#include <vector>
#include <openssl/aes.h>
#include "ProxyService/common/business_exception.h"
#include "ProxyService/common/error_code.h"

namespace ProxyService {

class AESTool {
public:
    typedef std::shared_ptr<AESTool> ptr;
    AESTool(const std::string &key) : key_(key) {
        if (key_.size() * 8 != 128) {
            throw BusinessException(AES_KEY_LEN_ERROR, getErrorMsg(AES_KEY_LEN_ERROR), __FILE__, __LINE__);
        }
    }

    std::string encrypt(const std::string &src) {
        AES_set_encrypt_key((uint8_t*)&*key_.begin(), 128, &aes_);

        int len = src.size(), newLen = 0;
        if ((len + 1) % 16 == 0) {
            // 长度刚好合适
            newLen = len + 1;
        }
        else {
            newLen = ((len + 1) / 16 + 1) * 16;
        }

        unsigned char iv[16] = {0};
        memset(iv, 'a', sizeof(iv));
        std::string encryptTemp(newLen, 0);
        AES_cbc_encrypt((uint8_t*)&*src.begin(), (uint8_t*)&*encryptTemp.begin(), newLen, &aes_, iv, AES_ENCRYPT);

        return encryptTemp;
    }

    std::string decrypt(const std::string &src) {
        AES_set_decrypt_key((uint8_t*)&*key_.begin(), 128, &aes_);

        int len = src.size(), oldLen = 0;
        if (src.size() % 16 == 0) {
            // 长度刚好合适
            oldLen = src.size() - 1;
        }
        else {
            oldLen = (len / 16 - 1) * 16 - 1;
        }

        unsigned char iv[16] = {0};
        memset(iv, 'a', sizeof(iv));
        std::string decryptTemp(len, 0);
        AES_cbc_encrypt((uint8_t*)&*src.begin(), (uint8_t*)&*decryptTemp.begin(), len, &aes_, iv, AES_DECRYPT);

        return std::string(decryptTemp.begin(), decryptTemp.begin() + oldLen);
    }

private:
    std::string key_;
    AES_KEY aes_;
};

}

#endif