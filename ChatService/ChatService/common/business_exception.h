/*************************************************************
 * business_exception.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2023-01-11 09:45:23
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
 *************************************************************/

#ifndef CHATSERVICE_COMMON_BUSINESS_EXCEPTION_H
#define CHATSERVICE_COMMON_BUSINESS_EXCEPTION_H

#include <exception>
#include <string>
#include <memory>
#include <sstream>
#include <corpc/common/log.h>

namespace ChatService {

class BusinessException : public std::exception {
public:
    BusinessException(long long code, const std::string &errInfo, const std::string &fileName, int line) : errorCode_(code), errorInfo_(errInfo), fileName_(fileName), line_(line) {
        USER_LOG_INFO << "[" << fileName_ << ":" << line << "] throw BusinessException: {code: " << errorCode_ << ", errinfo:" << errorInfo_ << "}";
    }
    ~BusinessException() {}

    const char *what() { return errorInfo_.c_str(); }

    std::string error() const { return errorInfo_; }
    long long code() const { return errorCode_; }
    std::string fileName() const { return fileName_; }
    int line() const { return line_; }

private:
    long long errorCode_{0};
    std::string errorInfo_;
    std::string fileName_;
    int line_{0};
};

}

#endif