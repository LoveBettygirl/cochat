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
#include <thread>
#include <corpc/net/tcp/tcp_client.h>
#include <corpc/net/load_balance.h>
#include <corpc/net/register/zk_service_register.h>
#include "TestClient/chat_service_codec.h"
#include "TestClient/chat_service_data.h"

namespace TestClient {

using json = nlohmann::json;

inline int32_t getInt32FromNetByte(const char *buf)
{
    int32_t temp;
    memcpy(&temp, buf, sizeof(temp));
    return ntohl(temp);
}

static corpc::TcpClient::ptr gClient;

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
        {"help", std::bind(&ChatClient::help, this, std::placeholders::_1)},
        {"showdata", std::bind(&ChatClient::showdata, this, std::placeholders::_1)},
        {"chat", std::bind(&ChatClient::chat, this, std::placeholders::_1)},
        {"addfriend", std::bind(&ChatClient::addfriend, this, std::placeholders::_1)},
        {"delfriend", std::bind(&ChatClient::delfriend, this, std::placeholders::_1)},
        {"creategroup", std::bind(&ChatClient::creategroup, this, std::placeholders::_1)},
        {"addgroup", std::bind(&ChatClient::addgroup, this, std::placeholders::_1)},
        {"quitgroup", std::bind(&ChatClient::quitgroup, this, std::placeholders::_1)},
        {"groupchat", std::bind(&ChatClient::groupchat, this, std::placeholders::_1)},
        {"logout", std::bind(&ChatClient::logout, this, std::placeholders::_1)}};
}

int ChatClient::sendClientRequest()
{
    ChatServiceStruct data;
    data.msgType = CLIENT_CONNECTION;
    ChatServiceCodeC codec;

    codec.encode(gClient->getConnection()->getOutBuffer(), &data);

    return gClient->sendData();
}

int ChatClient::sendKey(const std::string &str)
{
    ChatServiceStruct data;
    data.msgType = CLIENT_KEY;
    data.protocolData = rsa_->publicEncrypt(str);
    ChatServiceCodeC codec;

    codec.encode(gClient->getConnection()->getOutBuffer(), &data);

    return gClient->sendData();
}

int ChatClient::sendHeartbeat()
{
    ChatServiceStruct data;
    data.msgType = HEARTBEAT;
    ChatServiceCodeC codec;

    codec.encode(gClient->getConnection()->getOutBuffer(), &data);

    return gClient->sendData();
}

int ChatClient::sendMsg(const std::string &str)
{
    ChatServiceStruct data;
    data.msgType = CLIENT_MSG;
    data.protocolData = aes_->encrypt(str);
    ChatServiceCodeC codec;

    codec.encode(gClient->getConnection()->getOutBuffer(), &data);

    return gClient->sendData();
}

std::string ChatClient::recvMsg()
{
    std::shared_ptr<corpc::CustomStruct> data;
    int ret = gClient->recvData(data);
    if (ret != 0) {
        exit(-1);
    }
    std::shared_ptr<ChatServiceStruct> temp = std::dynamic_pointer_cast<ChatServiceStruct>(data);
    std::string finalStr;
    if (temp) {
        finalStr = temp->protocolData;
        if (temp->msgType == HEARTBEAT) {
            finalStr = "heartbeat";
        }
        return temp->msgType == CLIENT_MSG ? aes_->decrypt(finalStr) : finalStr;
    }
    return "";
}

void ChatClient::start()
{
    // 创建client端的socket
    corpc::IPAddress::ptr addr;
    if (port == 0 || ip == "0.0.0.0") {
        // 负载均衡
        corpc::AbstractServiceRegister::ptr center = std::make_shared<corpc::ZkServiceRegister>("127.0.0.1", 2181, 30000);
        std::vector<corpc::NetAddress::ptr> addrs = center->discoverService("ProxyService_Custom");
        corpc::LoadBalanceStrategy::ptr loadBalancer = corpc::LoadBalance::queryStrategy(corpc::LoadBalanceCategory::Random);
        addr = loadBalancer->select(addrs, corpc::PbStruct());
    }
    else {
        addr = std::make_shared<corpc::IPAddress>(ip, port);
    }
    gClient = std::make_shared<corpc::TcpClient>(addr, corpc::Custom_Protocol);
    gClient->setCustomCodeC(std::make_shared<ChatServiceCodeC>());
    gClient->setCustomData([]() {  return std::make_shared<ChatServiceStruct>(); });

    int ret = sendClientRequest();
    if (ret == -1) {
        std::cerr << "send client request error" << std::endl;
        exit(-1);
    }
    std::string serverPubKey = recvMsg();
    rsa_ = std::make_shared<RSATool>(serverPubKey);
    std::string randomAESKey = AESTool::randomString();
    aes_ = std::make_shared<AESTool>(randomAESKey);
    ret = sendKey(randomAESKey);
    if (ret == -1) {
        std::cerr << "send key error: " << randomAESKey << std::endl;
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem_, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(std::bind(&ChatClient::readTaskHandler, this)); // pthread_create
    readTask.detach(); // pthread_detach
    std::thread heatbeatTask(std::bind(&ChatClient::heartbeatTaskHandler, this)); // pthread_create
    heatbeatTask.detach(); // pthread_detach

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

            int len = sendMsg(request);
            if (len == -1) {
                std::cerr << "send login msg error: " << request << std::endl;
            }

            sem_wait(&rwsem_); // 等待信号量，由子线程处理完登录的响应消息后，通知这里
                
            if (isLoginSuccess_) {
                // 进入聊天主菜单页面
                isMainMenuRunning_ = true;
                mainMenu();
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

            int len = sendMsg(request);
            if (len == -1) {
                std::cerr << "send reg msg error: " << request << std::endl;
            }
            
            sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3: // quit业务
            sem_destroy(&rwsem_);
            exit(0);
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }
    }
}

// 主聊天页面程序
void ChatClient::mainMenu()
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
        it->second(commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void ChatClient::help(std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap_) {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}
// "addfriend" command handler
void ChatClient::addfriend(std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser_.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = sendMsg(buffer);
    if (len == -1) {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "delfriend" command handler
void ChatClient::delfriend(std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = DEL_FRIEND_MSG;
    js["id"] = currentUser_.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = sendMsg(buffer);
    if (len == -1) {
        std::cerr << "send delfriend msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "chat" command handler
void ChatClient::chat(std::string str)
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

    int len = sendMsg(buffer);
    if (len == -1) {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "creategroup" command handler  groupname:groupdesc
void ChatClient::creategroup(std::string str)
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

    int len = sendMsg(buffer);
    if (-1 == len) {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "addgroup" command handler
void ChatClient::addgroup(std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = currentUser_.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();

    int len = sendMsg(buffer);
    if (-1 == len) {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "quitgroup" command handler
void ChatClient::quitgroup(std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = QUIT_GROUP_MSG;
    js["id"] = currentUser_.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();

    int len = sendMsg(buffer);
    if (-1 == len) {
        std::cerr << "send quitgroup msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "groupchat" command handler   groupid:message
void ChatClient::groupchat(std::string str)
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

    int len = sendMsg(buffer);
    if (-1 == len) {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "logout" command handler
void ChatClient::logout(std::string)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = currentUser_.getId();
    std::string buffer = js.dump();

    int len = sendMsg(buffer);
    if (-1 == len) {
        std::cerr << "send logout msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
}
// "showdata" command handler
void ChatClient::showdata(std::string)
{
    json js;
    js["msgid"] = GET_USER_INFO_MSG;
    js["id"] = currentUser_.getId();
    std::string buffer = js.dump();

    int len = sendMsg(buffer);
    if (-1 == len) {
        std::cerr << "send logout msg error -> " << buffer << std::endl;
    }
    sem_wait(&rwsem_); // 等待信号量，子线程处理完注册消息会通知
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
    else {
        isMainMenuRunning_ = false;
        currentUser_ = User();
        currentUserFriendList_.clear();
        currentUserGroupList_.clear();
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
}

// 处理获取当前用户信息的响应逻辑
void ChatClient::doShowDataResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) {
        std::cerr << responsejs["errmsg"].get<std::string>() << std::endl;
    }
    else {
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
    }
}

// 子线程 - 接收线程
void ChatClient::readTaskHandler()
{
    while (true) {
        std::string buffer = recvMsg();
        if (buffer.empty()) {
            exit(-1);
        }
        if (buffer == "heartbeat") {
            continue;
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

        if (GET_USER_INFO_MSG_ACK == msgtype ) {
            doShowDataResponse(js);
            sem_post(&rwsem_);    // 通知主线程，登录结果处理完成
            continue;
        }
    }
}

// 发送心跳消息线程，每2s发一次
void ChatClient::heartbeatTaskHandler()
{
    while (true) {
        int len = sendHeartbeat();
        if (len == -1) {
            std::cerr << "send heartbeat msg error" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

// 显示当前登录成功用户的基本信息
void ChatClient::showCurrentUserData(std::string)
{
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