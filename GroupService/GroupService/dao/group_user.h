#ifndef GROUPSERVICE_DAO_GROUP_USER_H
#define GROUPSERVICE_DAO_GROUP_USER_H

#include "GroupService/dao/user.h"

namespace GroupService {

// 群组用户，多了一个role角色信息，从User类直接继承，复用User的其它信息
class GroupUser : public User {
public:
    void setRole(const std::string &role) { this->role = role; }
    std::string getRole() const { return this->role; }

private:
    std::string role;
};

}

#endif