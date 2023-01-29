#include "TestClient/chat_client.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace TestClient {

using json = nlohmann::json;

inline int32_t getInt32FromNetByte(const char *buf)
{
    int32_t temp;
    memcpy(&temp, buf, sizeof(temp));
    return ntohl(temp);
}

ChatClient::ChatClient(const std::string &ip, uint16_t port) : ip(ip), port(port)
{
    // 系统支持的客户端命令列表
    commandMap_ = {
        {"help", "显示所有支持的命令，格式help"},
        {"showdata", "显示当前登录成功用户的好友和所在群组信息，格式showdata"},
        {"chat", "一对一聊天，格式chat:friendid:message"},
        {"addfriend", "添加好友，格式addfriend:friendid"},
        {"delfriend", "删除好友，格式delfriend:friendid"},
        {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
        {"addgroup", "加入群组，格式addgroup:groupid"},
        {"quitgroup", "退出群组，格式quitgroup:groupid"},
        {"groupchat", "群聊，格式groupchat:groupid:message"},
        {"logout", "注销，格式logout"}};

    // 注册系统支持的客户端命令处理
    commandHandlerMap_ = {
        {"help", std::bind(&ChatClient::help, this, std::placeholders::_1, std::placeholders::_2)},
        {"showdata", std::bind(&ChatClient::showCurrentUserData, this, std::placeholders::_1, std::placeholders::_2)},
        {"chat", std::bind(&ChatClient::chat, this, std::placeholders::_1, std::placeholders::_2)},
        {"addfriend", std::bind(&ChatClient::addfriend, this, std::placeholders::_1, std::placeholders::_2)},
        {"delfriend", std::bind(&ChatClient::delfriend, this, std::placeholders::_1, std::placeholders::_2)},
        {"creategroup", std::bind(&ChatClient::creategroup, this, std::placeholders::_1, std::placeholders::_2)},
        {"addgroup", std::bind(&ChatClient::addgroup, this, std::placeholders::_1, std::placeholders::_2)},
        {"quitgroup", std::bind(&ChatClient::quitgroup, this, std::placeholders::_1, std::placeholders::_2)},
        {"groupchat", std::bind(&ChatClient::groupchat, this, std::placeholders::_1, std::placeholders::_2)},
        {"logout", std::bind(&ChatClient::logout, this, std::placeholders::_1, std::placeholders::_2)}};
}

int ChatClient::sendKey(int clientfd, const std::string &data)
{
    std::string encrypted = rsa_->publicEncrypt(data);

    int pkLen = 12 + encrypted.size();
    char *buf = reinterpret_cast<char *>(malloc(pkLen));
    char *temp = buf;

    int magicBegin = htonl(MAGIC_BEGIN);
    memcpy(temp, &magicBegin, sizeof(int32_t));

    int encryptedSize = htonl(encrypted.size());
    memcpy(temp, &encryptedSize, sizeof(int32_t));

    memcpy(temp, &*encrypted.begin(), encryptedSize);

    int magicEnd = htonl(MAGIC_END);
    memcpy(temp, &magicEnd, sizeof(int32_t));

    return send(clientfd, temp, pkLen + 12, 0);
}

int ChatClient::sendMsg(int clientfd, const std::string &data)
{
    std::string encrypted = aes_->encrypt(data);

    int pkLen = 12 + encrypted.size();
    char *buf = reinterpret_cast<char *>(malloc(pkLen));
    char *temp = buf;

    int magicBegin = htonl(MAGIC_BEGIN);
    memcpy(temp, &magicBegin, sizeof(int32_t));

    int encryptedSize = htonl(encrypted.size());
    memcpy(temp, &encryptedSize, sizeof(int32_t));

    memcpy(temp, &*encrypted.begin(), encryptedSize);

    int magicEnd = htonl(MAGIC_END);
    memcpy(temp, &magicEnd, sizeof(int32_t));

    return send(clientfd, temp, pkLen + 12, 0);
}

std::string ChatClient::recvMsg(int clientfd)
{
    char temp[2048] = {0};
    int32_t magic = -1, pkLen = -1;

    int len = recv(clientfd, temp, sizeof(int32_t), 0);
    if (-1 == len || 0 == len) {
        close(clientfd);
        exit(-1);
    }
    magic = getInt32FromNetByte(&temp[0]);
    if (magic != MAGIC_BEGIN) {
        close(clientfd);
        exit(-1);
    }

    len = recv(clientfd, temp + sizeof(int32_t), sizeof(int32_t), 0);
    if (-1 == len || 0 == len) {
        close(clientfd);
        exit(-1);
    }
    pkLen = getInt32FromNetByte(&temp[sizeof(int32_t)]);

    len = recv(clientfd, temp + 2 * sizeof(int32_t), pkLen, 0);
    if (-1 == len || 0 == len) {
        close(clientfd);
        exit(-1);
    }
    if (len != pkLen) {
        close(clientfd);
        exit(-1);
    }

    len = recv(clientfd, temp + 2 * sizeof(int32_t) + pkLen, sizeof(int32_t), 0);
    if (-1 == len || 0 == len) {
        close(clientfd);
        exit(-1);
    }
    magic = getInt32FromNetByte(&temp[2 * sizeof(int32_t) + pkLen]);
    if (magic != MAGIC_END) {
        close(clientfd);
        exit(-1);
    }

    return std::string(temp + 2 * sizeof(int32_t), temp + 2 * sizeof(int32_t) + pkLen);
}

void ChatClient::start()
{
    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd) {
        perror("socket create error");
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in))) {
        perror("connect server error");
        close(clientfd);
        exit(-1);
    }

    std::string serverPubKey = recvMsg(clientfd);
    rsa_ = std::make_shared<RSATool>(serverPubKey);
    std::string randomAESKey = AESTool::randomString();
    aes_ = std::make_shared<AESTool>(randomAESKey);
    int ret = sendKey(clientfd, randomAESKey);
    if (ret == -1) {
        std::cerr << "send key error: " << randomAESKey << std::endl;
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem_, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(std::bind(&ChatClient::readTaskHandler, this, std::placeholders::_1), clientfd); // pthread_create
    readTask.detach(); // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    while (true) {
        // 显示首页面菜单 登录、注册、退出
        std::cout << "========================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "choice: ";
        int choice = 0;
        std::cin >> choice;
        std::cin.get(); // 读掉缓冲区残留的回车

        switch (choice) {
        case 1: { // login业务
            int id = 0;
            char pwd[50] = {0};
            std::cout << "userid: ";
            std::cin >> id;
            std::cin.get(); // 读掉缓冲区残留的回车
            std::cout << "userpassword: ";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            std::string request = js.dump();

            isLoginSuccess_ = false;

            int len = sendMsg(clientfd, request);
            if (len == -1) {
                std::cerr << "send login msg error: " << request << std::endl;
            }

            sem_wait(&rwsem_); // 等待信号量，由子线程处理完登录的响应消息后，通知这里
                
            if (isLoginSuccess_) {
                // 进入聊天主菜单页面
                isMainMenuRunning_ = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: { // register业务
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout << "username: ";
            std::cin.getline(name, 50);
            std::cout << "userpassword: ";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = sendMsg(clientfd, request);
            if (len == -1) {
                std::cerr << "send reg msg error: " << request << std::endl;
            }
            
            sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3: // quit业务
            close(clientfd);
            sem_destroy(&rwsem_);
            exit(0);
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }
    }
}

// 主聊天页面程序
void ChatClient::mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning_) {
        std::cin.getline(buffer, sizeof(buffer));
        std::string commandbuf(buffer);
        std::string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx) {
            command = commandbuf;
        }
        else {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap_.find(command);
        if (it == commandHandlerMap_.end()) {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void ChatClient::help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap_) {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}
// "addfriend" command handler
void ChatClient::addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser_.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = sendMsg(clientfd, buffer);
    if (len == -1) {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "chat" command handler
void ChatClient::chat(int clientfd, std::string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx) {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = currentUser_.getId();
    js["name"] = currentUser_.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = sendMsg(clientfd, buffer);
    if (len == -1) {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "creategroup" command handler  groupname:groupdesc
void ChatClient::creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx) {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = currentUser_.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();

    int len = sendMsg(clientfd, buffer);
    if (-1 == len) {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "addgroup" command handler
void ChatClient::addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = currentUser_.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();

    int len = sendMsg(clientfd, buffer);
    if (-1 == len) {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "groupchat" command handler   groupid:message
void ChatClient::groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx) {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = currentUser_.getId();
    js["name"] = currentUser_.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = sendMsg(clientfd, buffer);
    if (-1 == len) {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "logout" command handler
void ChatClient::logout(int clientfd, std::string)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = currentUser_.getId();
    std::string buffer = js.dump();

    int len = sendMsg(clientfd, buffer);
    if (-1 == len) {
        std::cerr << "send logout msg error -> " << buffer << std::endl;
    }
    else {
        sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
        isMainMenuRunning_ = false;
        currentUser_ = User();
        currentUserFriendList_.clear();
        currentUserGroupList_.clear();
    }   
}

// 获取系统时间（聊天信息需要添加时间信息）
std::string ChatClient::getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// 处理注册的响应逻辑
void ChatClient::doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) { // 注册失败
        std::cerr << "name is already exist, register error!" << std::endl;
    }
    else { // 注册成功
        std::cout << "name register success, userid is " << responsejs["id"]
                << ", do not forget it!" << std::endl;
    }
}

// 处理登录的响应逻辑
void ChatClient::doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) { // 登录失败
        std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
        isLoginSuccess_ = false;
    }
    else { // 登录成功
        // 记录当前用户的id和name
        currentUser_ = User();
        currentUser_.setId(responsejs["id"].get<int>());
        currentUser_.setName(responsejs["name"]);
        currentUser_.setState("online");

        // 初始化
        currentUserFriendList_.clear();
        currentUserGroupList_.clear();

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends")) {
            std::vector<json> vec = responsejs["friends"];
            for (json &js : vec) {
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                currentUserFriendList_.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups")) {
            std::vector<json> vec1 = responsejs["groups"];
            for (json &grpjs : vec1) {
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<json> vec2 = grpjs["users"];
                for (json &js : vec2) {
                    GroupUser user;
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                currentUserGroupList_.push_back(group);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息  个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg")) {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for (std::string &str : vec) {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>()) {
                    std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                            << " said: " << js["msg"].get<std::string>() << std::endl;
                }
                else {
                    std::cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                            << " said: " << js["msg"].get<std::string>() << std::endl;
                }
            }
        }

        isLoginSuccess_ = true;
    }
}

// 处理注销的响应逻辑
void ChatClient::doLogoutResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) { // 注销失败
        std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
    }
}

// 处理单聊响应逻辑
void ChatClient::doOneChatResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) { // 聊天发送失败
        std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
    }
}

// 处理群聊响应逻辑
void ChatClient::doGroupChatResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) { // 聊天发送失败
        std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
    }
}

// 处理加好友响应逻辑
void ChatClient::doFriendResponse(json &responsejs)
{
    std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
    if (0 == responsejs["errno"].get<int>()) { // 加好友成功
        currentUserFriendList_.clear();
        std::vector<json> vec = responsejs["friends"];
        for (json &js : vec) {
            User user;
            user.setId(js["id"].get<int>());
            user.setName(js["name"]);
            user.setState(js["state"]);
            currentUserFriendList_.push_back(user);
        }
    }
}

// 处理群响应逻辑
void ChatClient::doGroupResponse(json &responsejs)
{
    std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
    if (0 == responsejs["errno"].get<int>()) { // 创建群成功
        currentUserGroupList_.clear();
        json groupjs = responsejs["group"];
        Group group(groupjs["id"], groupjs["groupname"], groupjs["groupdesc"]);
        std::vector<json> users = groupjs["users"];
        for (json &js : users) {
            GroupUser user;
            user.setId(js["id"].get<int>());
            user.setName(js["name"]);
            user.setState(js["state"]);
            user.setRole(js["role"]);
            group.getUsers().push_back(user);
        }
        currentUserGroupList_.push_back(group);
    }
}

// 子线程 - 接收线程
void ChatClient::readTaskHandler(int clientfd)
{
    while (true) {
        std::string buffer = recvMsg(clientfd);
        if (buffer.empty()) {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype) {
            std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                 << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype) {
            std::cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                 << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }

        if (ONE_CHAT_MSG_ACK == msgtype) {
            doOneChatResponse(js);
            sem_post(&rwsem_);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (GROUP_CHAT_MSG_ACK == msgtype) {
            doGroupChatResponse(js);
            sem_post(&rwsem_);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (LOGIN_MSG_ACK == msgtype) {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem_);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (LOGOUT_MSG_ACK == msgtype) {
            doLogoutResponse(js); // 处理注销响应的业务逻辑
            sem_post(&rwsem_);    // 通知主线程，注销结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype) {
            doRegResponse(js);
            sem_post(&rwsem_);    // 通知主线程，注册结果处理完成
            continue;
        }

        if (ADD_FRIEND_MSG_ACK == msgtype || DEL_FRIEND_MSG_ACK == msgtype) {
            doFriendResponse(js);
            sem_post(&rwsem_);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (ADD_GROUP_MSG_ACK == msgtype || CREATE_GROUP_MSG_ACK == msgtype || QUIT_GROUP_MSG_ACK == msgtype) {
            doGroupResponse(js);
            sem_post(&rwsem_);    // 通知主线程，登录结果处理完成
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
void ChatClient::showCurrentUserData(int, std::string)
{
    // TODO: 这边可以给服务端发请求，获取最新的信息，但是没空改了，之后再说吧
    std::cout << "======================login user======================" << std::endl;
    std::cout << "current login user => id: " << currentUser_.getId() << " name: " << currentUser_.getName() << std::endl;
    std::cout << "----------------------friend list---------------------" << std::endl;
    if (!currentUserFriendList_.empty()) {
        for (User &user : currentUserFriendList_) {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "----------------------group list----------------------" << std::endl;
    if (!currentUserGroupList_.empty()) {
        int i = 0;
        for (Group &group : currentUserGroupList_) {
            if (i > 0)
                std::cout << std::endl;
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers()) {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << std::endl;
            }
            i++;
        }
    }
    std::cout << "======================================================" << std::endl;
}

}