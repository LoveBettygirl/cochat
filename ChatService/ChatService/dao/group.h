#ifndef CHATSERVICE_DAO_GROUP_H
#define CHATSERVICE_DAO_GROUP_H

#include "ChatService/dao/group_user.h"
#include <string>
#include <vector>

namespace ChatService {

// Group表的ORM类
class Group {
public:
    Group(int id = -1, const std::string &name = "", const std::string &desc = "")
        : id(id), name(name), desc(desc) {}

    void setId(int id) { this->id = id; }
    void setName(const std::string &name) { this->name = name; }
    void setDesc(const std::string &desc) { this->desc = desc; }

    int getId() const { return this->id; }
    std::string getName() const { return this->name; }
    std::string getDesc() const { return this->desc; }
    std::vector<GroupUser> &getUsers() { return this->users; }

private:
    int id;
    std::string name;
    std::string desc;
    std::vector<GroupUser> users;
};

}

#endif