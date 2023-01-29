#ifndef TESTCLIENT_CHAT_CLIENT_H
#define TESTCLIENT_CHAT_CLIENT_H

#include "TestClient/lib/json.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <semaphore.h>
#include <atomic>
#include "TestClient/model/group.h"
#include "TestClient/model/user.h"
#include "TestClient/cypher/aes.h"
#include "TestClient/cypher/rsa.h"

namespace TestClient {

using json = nlohmann::json;

class ChatClient {
public:
    ChatClient(const std::string &ip, uint16_t port);
    void start();
private:
    std::string ip;

    uint16_t port;

    // 记录当前系统登录的用户信息
    User currentUser_;
    // 记录当前登录用户的好友列表信息
    std::vector<User> currentUserFriendList_;
    // 记录当前登录用户的群组列表信息
    std::vector<Group> currentUserGroupList_;

    // 控制主菜单页面程序
    bool isMainMenuRunning_ = false;

    // 用于读写线程之间的通信
    sem_t rwsem_;

    // 记录登录状态
    std::atomic_bool isLoginSuccess_{false};

    // 接收线程
    void readTaskHandler(int clientfd);

    // 获取系统时间（聊天信息需要添加时间信息）
    std::string getCurrentTime();

    // 主聊天页面程序
    void mainMenu(int);

    // 处理注册的响应逻辑
    void doRegResponse(json &responsejs);

    // 处理登录的响应逻辑
    void doLoginResponse(json &responsejs);

    // 处理注销的响应逻辑
    void doLogoutResponse(json &responsejs);

    // 处理加/删除好友响应逻辑
    void doFriendResponse(json &responsejs);

    // 处理群响应逻辑
    void doGroupResponse(json &responsejs);

    // 处理单聊响应逻辑
    void doOneChatResponse(json &responsejs);

    // 处理群聊响应逻辑
    void doGroupChatResponse(json &responsejs);

    /* handlers */

    // "help" command handler
    void help(int fd = 0, std::string str = "");
    // "showdata" command handler，显示当前登录成功用户的基本信息
    void showCurrentUserData(int fd = 0, std::string str = "");
    // "chat" command handler
    void chat(int, std::string);
    // "addfriend" command handler
    void addfriend(int, std::string);
    // "delfriend" command handler
    void delfriend(int, std::string);
    // "creategroup" command handler
    void creategroup(int, std::string);
    // "addgroup" command handler
    void addgroup(int, std::string);
    // "quitgroup" command handler
    void quitgroup(int, std::string);
    // "groupchat" command handler
    void groupchat(int, std::string);
    // "logout" command handler
    void logout(int, std::string);

    // 系统支持的客户端命令列表
    std::unordered_map<std::string, std::string> commandMap_;

    // 注册系统支持的客户端命令处理
    std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap_;

    AESTool::ptr aes_;
    RSATool::ptr rsa_;

    int sendMsg(int clientfd, const std::string &data);

    int sendKey(int clientfd, const std::string &data);

    std::string recvMsg(int clientfd);
};

}

#endif