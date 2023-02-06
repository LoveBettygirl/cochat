#ifndef CHATSERVICE_COMMON_MAGE_PACKAGE_H
#define CHATSERVICE_COMMON_MAGE_PACKAGE_H

#include <string>
#include <cstring>
#include <arpa/inet.h>
#include "ChatService/common/const.h"

namespace ChatService {

std::string makePackage(const std::string &data);

}

#endif