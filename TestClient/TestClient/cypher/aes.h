#ifndef PROXYSERVICE_CYPHER_AES_H
#define PROXYSERVICE_CYPHER_AES_H

#include <memory>
#include <string>
#include <vector>
#include <openssl/aes.h>
#include <iostream>
#include <cstring>
#include <random>

namespace TestClient {

class AESTool {
public:
    typedef std::shared_ptr<AESTool> ptr;
    AESTool(const std::string &key) : key_(key) {
        if (key_.size() * 8 != 128) {
            std::cerr << "aes key len error" << std::endl;
            exit(-1);
        }
    }

    std::string encrypt(const std::string &src) {
        AES_set_encrypt_key((uint8_t*)&*key_.begin(), 128, &aes_);

        std::string srcTemp = PKCS7Padding(src, 16);

        unsigned char iv[16] = {0};
        memset(iv, 'a', sizeof(iv));
        std::string encryptTemp(srcTemp.size(), 0);
        AES_cbc_encrypt((uint8_t*)&*srcTemp.begin(), (uint8_t*)&*encryptTemp.begin(), srcTemp.size(), &aes_, iv, AES_ENCRYPT);

        return encryptTemp;
    }

    std::string decrypt(const std::string &src) {
        AES_set_decrypt_key((uint8_t*)&*key_.begin(), 128, &aes_);

        unsigned char iv[16] = {0};
        memset(iv, 'a', sizeof(iv));
        std::string decryptTemp(src.size(), 0);
        AES_cbc_encrypt((uint8_t*)&*src.begin(), (uint8_t*)&*decryptTemp.begin(), src.size(), &aes_, iv, AES_DECRYPT);

        return PKCS7UnPadding(decryptTemp);
    }

    static std::string randomString(std::string::size_type len = 16) {
        static std::string alphaNumeric("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        std::string result;
        result.reserve(len);
        std::random_device rd;
        std::mt19937 generator(rd());
        std::string source(alphaNumeric);
        std::string::size_type generated = 0;
        while (generated < len) {
            std::shuffle(source.begin(), source.end(), generator);
            std::string::size_type delta = std::min({len - generated, source.length()});
            result.append(source.substr(0, delta));
            generated += delta;
        }
        return result;
    }

private:
    std::string PKCS7Padding(const std::string &in, int alignSize) {
        // 计算需要填充字节数（按alignSize字节对齐进行填充）
        int remainder = in.size() % alignSize;
        int paddingSize = (remainder == 0) ? alignSize : (alignSize - remainder);

        // 进行填充
        std::string temp(in);
        temp.insert(temp.size(), paddingSize, paddingSize);
        return temp;
    }

    std::string PKCS7UnPadding(const std::string &in) {
        char paddingSize = in.at(in.size() - 1);
        return std::string(in.begin(), in.begin() + in.size() - paddingSize);
    }

private:
    std::string key_;
    AES_KEY aes_;
};

}

#endif