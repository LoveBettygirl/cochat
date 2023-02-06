#ifndef USERSERVICE_MODEL_USER_H
#define USERSERVICE_MODEL_USER_H

#include <string>
#include "UserService/common/const.h"

namespace UserService {

// User表的ORM类
class User {
public:
    User(int id = -1, const std::string &name = "", const std::string &pwd = "", const std::string &salt = "", const std::string &state = OFFLINE_STATE)
        : id(id), name(name), password(pwd), state(state) {}
    
    void setId(int id) { this->id = id; }
    void setName(const std::string &name) { this->name = name; }
    void setPwd(const std::string &pwd) { this->password = pwd; }
    void setState(const std::string &state) { this->state = state; }
    void setSalt(const std::string &salt) { this->salt = salt; }

    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getPwd() const { return password; }
    std::string getState() const { return state; }
    std::string getSalt() const { return salt; }
private:
    int id;
    std::string name;
    std::string password;
    std::string state;
    std::string salt;
};

}

#endif