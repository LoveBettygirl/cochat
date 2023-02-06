#ifndef PROXYSERVICE_CYPHER_RSA_H
#define PROXYSERVICE_CYPHER_RSA_H

#include <memory>
#include <string>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <corpc/common/log.h>
#include "ProxyService/common/business_exception.h"

namespace ProxyService {

class RSATool {
public:
    typedef std::shared_ptr<RSATool> ptr;
    RSATool(const std::string &key, bool isPrivate = false) {
        if (isPrivate) {
            readPrivateKey(key);
        }
        else {
            readPublicKey(key);
        }
    }

    ~RSATool() {
        if (publicRsa_) {
            RSA_free(publicRsa_);
        }
        if (privateRsa_) {
            RSA_free(privateRsa_);
        }
    }

    std::string publicEncrypt(const std::string &src) {
        int rsaLen = RSA_size(publicRsa_);
        std::string encryptMsg(rsaLen, 0);
        int len = rsaLen - 11;
        int msgLen = RSA_public_encrypt(len, (uint8_t*)&*src.begin(), (uint8_t*)&*encryptMsg.begin(), publicRsa_, RSA_PKCS1_PADDING);
        if (msgLen < 0)
            return "";
        return encryptMsg.substr(0, msgLen);
    }

    std::string privateDecrypt(const std::string &src) {
        int rsaLen = RSA_size(privateRsa_);
        std::string decryptMsg(rsaLen, 0);
        int msgLen = RSA_private_decrypt(rsaLen, (uint8_t*)&*src.begin(), (uint8_t*)&*decryptMsg.begin(), privateRsa_, RSA_PKCS1_PADDING);
        if (msgLen < 0)
            return "";
        return decryptMsg.substr(0, msgLen);
    }

private:
    bool readPrivateKey(const std::string &key) {
        BIO *bio = nullptr;
        if ((bio = BIO_new_mem_buf((void*)&*key.begin(), -1)) == nullptr) {
            USER_LOG_ERROR << "BIO_new_mem_buf privateKey error";
            return false;
        } 	
        if ((privateRsa_ = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr)) == nullptr) {
            USER_LOG_ERROR << "PEM_read_bio_RSAPrivateKey error";
            return false;
        }
        BIO_free_all(bio);
        return true;
    }

    bool readPublicKey(const std::string &key) {
        BIO *bio = nullptr;
        if ((bio = BIO_new_mem_buf((void*)&*key.begin(), -1)) == nullptr) {
            USER_LOG_ERROR << "BIO_new_mem_buf publicKey error";
            return false;
        }
        if ((publicRsa_ = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr)) == nullptr)  {
            USER_LOG_ERROR << "PEM_read_bio_RSA_PUBKEY error";
            return false;
        }
        BIO_free_all(bio);
        return true;
    }

private:
    RSA *publicRsa_{nullptr};
    RSA *privateRsa_{nullptr};
    bool isPrivate_;
};

}

#endif