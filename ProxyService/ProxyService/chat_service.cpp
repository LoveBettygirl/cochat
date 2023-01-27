#include "ProxyService/chat_service.h"
#include <string>
#include <map>
#include <fstream>
#include <corpc/common/start.h>
#include "ProxyService/common/error_code.h"
#include "ProxyService/common/const.h"
#include <corpc/net/pb/pb_rpc_channel.h>
#include <corpc/net/pb/pb_rpc_controller.h>
#include <corpc/net/service_register.h>

namespace ProxyService {

// 获取单例对象的接口函数，线程安全的
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的回调操作
ChatService::ChatService()
{
    // 网络模块和业务模块解耦的核心
    // 用户基本业务管理相关事件处理回调注册
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({DEL_FRIEND_MSG, std::bind(&ChatService::deleteFriend, this, std::placeholders::_1, std::placeholders::_2)});

    // 群组业务管理相关事件处理回调注册
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({QUIT_GROUP_MSG, std::bind(&ChatService::quitGroup, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2)});

    msgHandlerMap_.insert({FORWARDED_MSG, std::bind(&ChatService::forwarded, this, std::placeholders::_1, std::placeholders::_2)});

    center_ = corpc::ServiceRegister::queryRegister(corpc::getConfig()->serviceRegister);

    readRsaKey(corpc::getConfig()->getYamlNode("rsa")["private_key_path"].as<std::string>(), rsaPrivateKey);
    readRsaKey(corpc::getConfig()->getYamlNode("rsa")["public_key_path"].as<std::string>(), rsaPublicKey);

    rsa = std::make_shared<RSATool>(rsaPrivateKey, true);
}

void ChatService::readRsaKey(const std::string &fileName, std::string &pemFile)
{
    std::ifstream in(fileName);
    if (!in.is_open()) {
        USER_LOG_FATAL << "pem file not exist";
    }

    std::string temp;
    while (std::getline(in, temp)) {
        pemFile += temp + "\n";
    }
}

void ChatService::sendPubKeyToClient(const corpc::TcpConnection::ptr &conn)
{
    if (userCipherMap_.find(conn) == userCipherMap_.end()) {
        userCipherMap_.insert({conn, nullptr});
    }
    sendMsg(conn, rsaPublicKey);
}

void ChatService::dealWithClient(const corpc::TcpConnection::ptr &conn, const std::string &data)
{
    auto it = userCipherMap_.find(conn);
    if (it != userCipherMap_.end() && it->second == nullptr) {
        std::string aesKey = rsa->privateDecrypt(data);
        userCipherMap_[conn] = std::make_shared<AESTool>(aesKey);
        return;
    }

    // 解码数据
    std::string strs = userCipherMap_[conn]->decrypt(data);

    // 数据的反序列化
    json js = json::parse(strs);
    // 目的：完全解耦网络模块和业务模块的代码，充分利用oop，C++中的接口就是抽象基类
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js);
}

void ChatService::sendMsg(const corpc::TcpConnection::ptr &conn, const std::string &data)
{
    std::string encrypted = userCipherMap_[conn]->encrypt(data);

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

    conn->send(temp, pkLen);
}

void ChatService::sendMsgInCor(const corpc::TcpConnection::ptr &conn, const std::string &data)
{
    std::string encrypted = userCipherMap_[conn]->encrypt(data);

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

    conn->sendInCor(temp, pkLen);
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end()) {
        // 返回一个默认的处理器，空操作
        return [=](const corpc::TcpConnection::ptr &conn, json &js) {
            USER_LOG_ERROR << "msgid: " << msgid << " cannot find handler!";
        };
    }
    return msgHandlerMap_[msgid];
}

// 处理登录业务
void ChatService::login(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do login service!!!";

    int id = js["id"].get<int>();

    LoginRequest loginReq;
    LoginResponse loginRes;
    loginReq.set_user_id(id);
    loginReq.set_user_password(js["password"]);
    loginReq.set_auth_info(corpc::getServer()->getLocalAddr()->toString());
    int ret = doLoginReq(loginReq, loginRes);

    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    response["errno"] = loginRes.ret_code();
    response["errmsg"] = loginRes.res_info();

    if (loginRes.ret_code() == ProxyService::SUCCESS) {
        // 登录成功，记录用户连接信息(要考虑多用户并发登录的时候，线程安全问题，互斥锁)
        {
            // 临界区
            std::unique_lock<std::mutex> lock(connMutex_);
            userConnMap_.insert({id, conn});
        }

        response["id"] = id;

        // 查询用户信息
        UserInfoRequest userInfoReq;
        UserInfoResponse userInfoRes;
        userInfoReq.set_user_id(id);
        int ret = doGetUserInfoReq(userInfoReq, userInfoRes);
        if (userInfoRes.ret_code() == ProxyService::SUCCESS) {
            response["name"] = userInfoRes.user().name();
        }
        else {
            response["errno"] = userInfoRes.ret_code();
            response["errmsg"] = userInfoRes.res_info();
        }

        // 查询该用户是否有离线消息
        ReadOfflineMessageRequest readOfflineMessageReq;
        ReadOfflineMessageResponse readOfflineMessageRes;
        readOfflineMessageReq.set_user_id(id);
        ret = doReadOfflineMessageReq(readOfflineMessageReq, readOfflineMessageRes);
        std::vector<std::string> vec;
        if (readOfflineMessageRes.ret_code() == ProxyService::SUCCESS) {
            auto &msgs = readOfflineMessageRes.msgs();
            for (auto &msg : msgs) {
                vec.push_back(msg);
            }
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
            }
        }
        else {
            response["errno"] = readOfflineMessageRes.ret_code();
            response["errmsg"] = readOfflineMessageRes.res_info();
        }

        // 查询该用户的好友信息并返回
        FriendListRequest friendListReq;
        FriendListResponse friendListRes;
        friendListReq.set_user_id(id);
        ret = doGetFriendListReq(friendListReq, friendListRes);
        if (friendListRes.ret_code() == ProxyService::SUCCESS) {
            std::vector<json> vec2;
            for (auto &user : friendListRes.friends()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                js["state"] = user.state();
                vec2.push_back(js);
            }
            response["friends"] = vec2;
        }
        else {
            response["errno"] = friendListRes.ret_code();
            response["errmsg"] = friendListRes.res_info();
        }

        // 查询用户的群组信息
        GetUserGroupsRequest userGroupsReq;
        GetUserGroupsResponse userGroupsRes;
        userGroupsReq.set_user_id(id);
        ret = doGetUserGroupsReq(userGroupsReq, userGroupsRes);
        if (userGroupsRes.ret_code() == ProxyService::SUCCESS) {
            // group:[{groupid:[xxx, xxx, xxx, xxx]}]
            std::vector<json> groupV;
            for (auto &group : userGroupsRes.groups()) {
                json grpjson;
                grpjson["id"] = group.id();
                grpjson["groupname"] = group.name();
                grpjson["groupdesc"] = group.desc();
                std::vector<json> userV;
                for (auto &user : group.users()) {
                    json js;
                    js["id"] = user.id();
                    js["name"] = user.name();
                    js["state"] = user.state();
                    js["role"] = user.role();
                    userV.push_back(js);
                }
                grpjson["users"] = userV;
                groupV.push_back(grpjson);
            }
            response["groups"] = groupV;
        }
        else {
            response["errno"] = userGroupsRes.ret_code();
            response["errmsg"] = userGroupsRes.res_info();
        }
    }

    sendMsg(conn, response.dump());
}

// 处理注册业务
void ChatService::reg(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do reg service!!!";
    std::string name = js["name"];
    std::string pwd = js["password"];

    RegisterRequest registerReq;
    RegisterResponse registerRes;
    registerReq.set_user_name(name);
    registerReq.set_user_password(pwd);
    doRegisterReq(registerReq, registerRes);

    json response;
    if (registerRes.ret_code() == ProxyService::SUCCESS) {
        response["id"] = registerRes.user_id();
    }

    response["msgid"] = REG_MSG_ACK;
    response["errno"] = registerRes.ret_code();
    response["errmsg"] = registerRes.res_info();
    sendMsg(conn, response.dump());
}

void ChatService::oneChat(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do one chat service!!!";
    int fromid = js["id"].get<int>();
    int toid = js["to"].get<int>();

    json response;
    response["msgid"] = ONE_CHAT_MSG_ACK;
    {
        std::unique_lock<std::mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) {
            // toid登录在本机，转发消息
            // 服务器主动推送消息给toid用户
            sendMsgInCor(it->second, js.dump());
            response["errno"] = ProxyService::SUCCESS;
            response["errmsg"] = ProxyService::getErrorMsg(SUCCESS);
            sendMsg(conn, response.dump());
            return;
        }
    }

    OneChatRequest oneChatReq;
    OneChatResponse oneChatRes;
    oneChatReq.set_from_user_id(fromid);
    oneChatReq.set_to_user_id(toid);
    oneChatReq.set_msg(js.dump());
    int ret = doOneChatReq(oneChatReq, oneChatRes);
    response["errno"] = oneChatRes.ret_code();
    response["errmsg"] = oneChatRes.res_info();
    sendMsg(conn, response.dump());
}

// 添加好友业务
void ChatService::addFriend(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do add friend service!!!";
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    AddFriendRequest addFriendReq;
    AddFriendResponse addFriendRes;
    addFriendReq.set_user_id(userid);
    addFriendReq.set_friend_id(friendid);
    int ret = doAddFriendReq(addFriendReq, addFriendRes);
    
    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;
    response["errno"] = addFriendRes.ret_code();
    response["errmsg"] = addFriendRes.res_info();
    // 存储好友信息
    if (addFriendRes.ret_code() == ProxyService::SUCCESS) {
        FriendListRequest friendListReq;
        FriendListResponse friendListRes;
        friendListReq.set_user_id(friendid);
        int ret = doGetFriendListReq(friendListReq, friendListRes);
        if (friendListRes.ret_code() == ProxyService::SUCCESS) {
            std::vector<json> vec2;
            for (auto &user : friendListRes.friends()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                js["state"] = user.state();
                vec2.push_back(js);
            }
            response["friends"] = vec2;
        }
        else {
            response["errno"] = friendListRes.ret_code();
            response["errmsg"] = friendListRes.res_info();
        }
    }
    sendMsg(conn, response.dump());
}

// 删除好友业务
void ChatService::deleteFriend(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do delete friend service!!!";
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    DeleteFriendRequest delFriendReq;
    DeleteFriendResponse delFriendRes;
    delFriendReq.set_user_id(userid);
    delFriendReq.set_friend_id(friendid);
    int ret = doDeleteFriendReq(delFriendReq, delFriendRes);
    
    json response;
    response["msgid"] = DEL_FRIEND_MSG_ACK;
    response["errno"] = delFriendRes.ret_code();
    response["errmsg"] = delFriendRes.res_info();
    if (delFriendRes.ret_code() == ProxyService::SUCCESS) {
        FriendListRequest friendListReq;
        FriendListResponse friendListRes;
        friendListReq.set_user_id(friendid);
        int ret = doGetFriendListReq(friendListReq, friendListRes);
        if (friendListRes.ret_code() == ProxyService::SUCCESS) {
            std::vector<json> vec2;
            for (auto &user : friendListRes.friends()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                js["state"] = user.state();
                vec2.push_back(js);
            }
            response["friends"] = vec2;
        }
        else {
            response["errno"] = friendListRes.ret_code();
            response["errmsg"] = friendListRes.res_info();
        }
    }
    sendMsg(conn, response.dump());
}

// 处理客户端异常退出
void ChatService::clientCloseException(const corpc::TcpConnection::ptr &conn)
{
    int userid = -1;
    {
        // 临界区
        std::unique_lock<std::mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it) {
            if (it->second == conn) {
                // 从map表删除用户的连接信息
                userid = it->first;
                userConnMap_.erase(it);
                userCipherMap_.erase(userCipherMap_.find(conn));
                break;
            }
        }
    }

    // 用户注销，相当于下线
    LogoutRequest logoutReq;
    LogoutResponse logoutRes;
    logoutReq.set_user_id(userid);
    int ret = doLogoutReq(logoutReq, logoutRes);
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把（当前服务器上）online状态的用户，设置成offline
    // _userModel.resetState();
    // 临界区
    std::unique_lock<std::mutex> lock(connMutex_);
    for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it) {
        int userid = it->first;
        // 用户注销，相当于下线
        LogoutRequest logoutReq;
        LogoutResponse logoutRes;
        logoutReq.set_user_id(userid);
        int ret = doLogoutReq(logoutReq, logoutRes);
    }
    userConnMap_.clear();
    userCipherMap_.clear();
}

// 创建群组业务
void ChatService::createGroup(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do create group service!!!";
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    CreateGroupRequest createGroupReq;
    CreateGroupResponse createGroupRes;
    createGroupReq.set_user_id(userid);
    createGroupReq.set_group_name(name);
    createGroupReq.set_group_desc(desc);
    int ret = doCreateGroupReq(createGroupReq, createGroupRes);

    json response;
    response["msgid"] = CREATE_GROUP_MSG_ACK;
    response["errno"] = createGroupRes.ret_code();
    response["errmsg"] = createGroupRes.res_info();
    if (createGroupRes.ret_code() == ProxyService::SUCCESS) {
        GetGroupInfoRequest getGroupInfoReq;
        GetGroupInfoResponse getGroupInfoRes;
        getGroupInfoReq.set_group_id(createGroupRes.group_id());
        ret = doGetGroupInfoReq(getGroupInfoReq, getGroupInfoRes);
        if (getGroupInfoRes.ret_code() == ProxyService::SUCCESS) {
            json groupjs;
            groupjs["id"] = getGroupInfoRes.group_info().id();
            groupjs["groupname"] = getGroupInfoRes.group_info().name();
            groupjs["groupdesc"] = getGroupInfoRes.group_info().desc();
            std::vector<json> userV;
            for (auto &user : getGroupInfoRes.group_info().users()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                js["state"] = user.state();
                js["role"] = user.role();
                userV.push_back(js);
            }
            groupjs["users"] = userV;
            response["group"] = groupjs;
        }
        else {
            response["errno"] = getGroupInfoRes.ret_code();
            response["errmsg"] = getGroupInfoRes.res_info();
        }
    }
    sendMsg(conn, response.dump());
}

// 加入群组业务
void ChatService::addGroup(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do add group service!!!";
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    AddGroupRequest addGroupReq;
    AddGroupResponse addGroupRes;
    addGroupReq.set_user_id(userid);
    addGroupReq.set_group_id(groupid);
    int ret = doAddGroupReq(addGroupReq, addGroupRes);

    json response;
    response["msgid"] = ADD_GROUP_MSG_ACK;
    response["errno"] = addGroupRes.ret_code();
    response["errmsg"] = addGroupRes.res_info();
    if (addGroupRes.ret_code() == ProxyService::SUCCESS) {
        GetGroupInfoRequest getGroupInfoReq;
        GetGroupInfoResponse getGroupInfoRes;
        getGroupInfoReq.set_group_id(groupid);
        ret = doGetGroupInfoReq(getGroupInfoReq, getGroupInfoRes);
        if (getGroupInfoRes.ret_code() == ProxyService::SUCCESS) {
            json groupjs;
            groupjs["id"] = getGroupInfoRes.group_info().id();
            groupjs["groupname"] = getGroupInfoRes.group_info().name();
            groupjs["groupdesc"] = getGroupInfoRes.group_info().desc();
            std::vector<json> userV;
            for (auto &user : getGroupInfoRes.group_info().users()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                js["state"] = user.state();
                js["role"] = user.role();
                userV.push_back(js);
            }
            groupjs["users"] = userV;
            response["group"] = groupjs;
        }
    }
    sendMsg(conn, response.dump());
}

// 退出群组业务
void ChatService::quitGroup(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do quit group service!!!";
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    QuitGroupRequest quitGroupReq;
    QuitGroupResponse quitGroupRes;
    quitGroupReq.set_user_id(userid);
    quitGroupReq.set_group_id(groupid);
    int ret = doQuitGroupReq(quitGroupReq, quitGroupRes);

    json response;
    response["msgid"] = QUIT_GROUP_MSG_ACK;
    response["errno"] = quitGroupRes.ret_code();
    response["errmsg"] = quitGroupRes.res_info();
    if (quitGroupRes.ret_code() == ProxyService::SUCCESS) {
        GetGroupInfoRequest getGroupInfoReq;
        GetGroupInfoResponse getGroupInfoRes;
        getGroupInfoReq.set_group_id(groupid);
        ret = doGetGroupInfoReq(getGroupInfoReq, getGroupInfoRes);
        if (getGroupInfoRes.ret_code() == ProxyService::SUCCESS) {
            json groupjs;
            groupjs["id"] = getGroupInfoRes.group_info().id();
            groupjs["groupname"] = getGroupInfoRes.group_info().name();
            groupjs["groupdesc"] = getGroupInfoRes.group_info().desc();
            std::vector<json> userV;
            for (auto &user : getGroupInfoRes.group_info().users()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                js["state"] = user.state();
                js["role"] = user.role();
                userV.push_back(js);
            }
            groupjs["users"] = userV;
            response["group"] = groupjs;
        }
    }
    sendMsg(conn, response.dump());
}

// 群组聊天业务
void ChatService::groupChat(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do group chat service!!!";
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    GroupChatRequest groupChatReq;
    GroupChatResponse groupChatRes;
    groupChatReq.set_from_user_id(userid);
    groupChatReq.set_to_group_id(groupid);
    int ret = doGroupChatReq(groupChatReq, groupChatRes);

    json response;
    response["msgid"] = GROUP_CHAT_MSG_ACK;
    response["errno"] = groupChatRes.ret_code();
    response["errmsg"] = groupChatRes.res_info();
    sendMsg(conn, response.dump());
}

// 处理注销业务
void ChatService::logout(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do logout service!!!";
    int userid = js["id"].get<int>();

    LogoutRequest logoutReq;
    LogoutResponse logoutRes;
    logoutReq.set_user_id(userid);
    int ret = doLogoutReq(logoutReq, logoutRes);

    json response;
    response["msgid"] = LOGOUT_MSG_ACK;
    response["errno"] = logoutRes.ret_code();
    response["errmsg"] = logoutRes.res_info();

    if (logoutRes.ret_code() == ProxyService::SUCCESS) {
        {
            // 临界区
            std::unique_lock<std::mutex> lock(connMutex_);
            auto it = userConnMap_.find(userid);
            if (it != userConnMap_.end()) {
                userConnMap_.erase(it);
            }
        }
    }

    sendMsg(conn, response.dump());
    userCipherMap_.erase(userCipherMap_.find(conn));
}

// 处理转发过来的消息
void ChatService::forwarded(const corpc::TcpConnection::ptr &conn, json &js)
{
    int toid = js["to"].get<int>();
    {
        // 临界区
        std::unique_lock<std::mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) {
            sendMsgInCor(it->second, js["msg"].get<std::string>());
            return;
        }
    }
}

int ChatService::doLoginReq(const ::LoginRequest &request, ::LoginResponse &response)
{
    corpc::AbstractServiceRegister::ptr center = corpc::ServiceRegister::queryRegister(corpc::getConfig()->serviceRegister);
    std::vector<corpc::NetAddress::ptr> addrs = center->discoverService("UserServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    UserServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.Login(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doLogoutReq(const ::LogoutRequest &request, ::LogoutResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("UserServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    UserServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.Logout(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doRegisterReq(const ::RegisterRequest &request, ::RegisterResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("UserServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    UserServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.Register(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doAddFriendReq(const ::AddFriendRequest &request, ::AddFriendResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("FriendServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    FriendServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.AddFriend(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doDeleteFriendReq(::DeleteFriendRequest &request, ::DeleteFriendResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("FriendServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    FriendServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.DeleteFriend(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doGetUserInfoReq(const ::UserInfoRequest &request, ::UserInfoResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("FriendServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    FriendServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.GetUserInfo(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doGetFriendListReq(const ::FriendListRequest &request, ::FriendListResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("FriendServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    FriendServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.GetFriendList(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doCreateGroupReq(const ::CreateGroupRequest &request, ::CreateGroupResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("GroupServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    GroupServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.CreateGroup(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doAddGroupReq(const ::AddGroupRequest &request, ::AddGroupResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("GroupServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    GroupServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.AddGroup(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doQuitGroupReq(const ::QuitGroupRequest &request, ::QuitGroupResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("GroupServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    GroupServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.QuitGroup(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doGetGroupInfoReq(const ::GetGroupInfoRequest &request, ::GetGroupInfoResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("GroupServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    GroupServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.GetGroupInfo(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doGetUserGroupsReq(const ::GetUserGroupsRequest &request, ::GetUserGroupsResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("GroupServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    GroupServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.GetUserGroups(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doOneChatReq(const ::OneChatRequest &request, ::OneChatResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("ChatServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    ChatServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.OneChat(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doGroupChatReq(const ::GroupChatRequest &request, ::GroupChatResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("ChatServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    ChatServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.GroupChat(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doReadOfflineMessageReq(const ::ReadOfflineMessageRequest &request, ::ReadOfflineMessageResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("ChatServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    ChatServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.ReadOfflineMessage(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

int ChatService::doSaveOfflineMessageReq(const ::SaveOfflineMessageRequest &request, ::SaveOfflineMessageResponse &response)
{
    std::vector<corpc::NetAddress::ptr> addrs = center_->discoverService("ChatServiceRpc");

    corpc::PbRpcChannel channel(addrs, corpc::LoadBalanceCategory::ConsistentHash);
    ChatServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    stub.SaveOfflineMessage(&rpcController, &request, &response, nullptr);

    if (rpcController.ErrorCode() != 0) {
        return -1;
    }
    return 0;
}

}