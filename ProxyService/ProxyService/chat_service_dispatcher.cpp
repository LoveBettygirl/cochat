#include "ProxyService/chat_service_dispatcher.h"
#include "ProxyService/chat_service_codec.h"
#include "ProxyService/chat_service_data.h"
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

// 注册消息以及对应的回调操作
ChatServiceDispatcher::ChatServiceDispatcher()
{
    // 网络模块和业务模块解耦的核心
    // 用户基本业务管理相关事件处理回调注册
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatServiceDispatcher::login, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({LOGOUT_MSG, std::bind(&ChatServiceDispatcher::logout, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({REG_MSG, std::bind(&ChatServiceDispatcher::reg, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(&ChatServiceDispatcher::oneChat, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatServiceDispatcher::addFriend, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({DEL_FRIEND_MSG, std::bind(&ChatServiceDispatcher::deleteFriend, this, std::placeholders::_1, std::placeholders::_2)});

    // 群组业务管理相关事件处理回调注册
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatServiceDispatcher::createGroup, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(&ChatServiceDispatcher::addGroup, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({QUIT_GROUP_MSG, std::bind(&ChatServiceDispatcher::quitGroup, this, std::placeholders::_1, std::placeholders::_2)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatServiceDispatcher::groupChat, this, std::placeholders::_1, std::placeholders::_2)});

    msgHandlerMap_.insert({FORWARDED_MSG, std::bind(&ChatServiceDispatcher::forwarded, this, std::placeholders::_1, std::placeholders::_2)});

    msgHandlerMap_.insert({GET_USER_INFO_MSG, std::bind(&ChatServiceDispatcher::getUserInfo, this, std::placeholders::_1, std::placeholders::_2)});

    center_ = corpc::ServiceRegister::queryRegister(corpc::getConfig()->serviceRegister);

    readRsaKey(corpc::getConfig()->getYamlNode("rsa")["private_key_path"].as<std::string>(), rsaPrivateKey_);
    readRsaKey(corpc::getConfig()->getYamlNode("rsa")["public_key_path"].as<std::string>(), rsaPublicKey_);

    rsa_ = std::make_shared<RSATool>(rsaPrivateKey_, true);
}

void ChatServiceDispatcher::readRsaKey(const std::string &fileName, std::string &pemFile)
{
    std::ifstream in(fileName);
    if (!in.is_open()) {
        USER_LOG_FATAL << "pem file not exist";
    }

    std::stringstream ss;
    ss << in.rdbuf();
    pemFile = ss.str();
}

void ChatServiceDispatcher::dispatch(corpc::AbstractData *data, const corpc::TcpConnection::ptr &conn)
{
    ChatServiceStruct *request = dynamic_cast<ChatServiceStruct *>(data);
    ChatServiceStruct response;
    if (request->msgType == CLIENT_CONNECTION) {
        {
            std::unique_lock<std::mutex> lock(cipherMutex_);
            if (userCipherMap_.find(conn) == userCipherMap_.end()) {
                userCipherMap_.insert({conn, nullptr});
            }
        }
        response.msgType = SERVER_PUBKEY;
        response.protocolData = rsaPublicKey_;
        conn->getCodec()->encode(conn->getOutBuffer(), &response);
        return;
    }
    else if (request->msgType == CLIENT_KEY) {
        std::unique_lock<std::mutex> lock(cipherMutex_);
        auto it = userCipherMap_.find(conn);
        if (it != userCipherMap_.end() && it->second == nullptr) {
            std::string aesKey = rsa_->privateDecrypt(request->protocolData);
            userCipherMap_[conn] = std::make_shared<AESTool>(aesKey);
        }
        return;
    }
    else if (request->msgType == CLIENT_MSG || request->msgType == SERVER_FORWARD_MSG) {
        // 解码数据
        std::string strs = request->protocolData;
        if (request->msgType == CLIENT_MSG) {
            std::unique_lock<std::mutex> lock(cipherMutex_);
            strs = userCipherMap_[conn]->decrypt(strs);
        }

        // 数据的反序列化
        json js = json::parse(strs);
        // 目的：完全解耦网络模块和业务模块的代码，充分利用oop，C++中的接口就是抽象基类
        auto msgHandler = getHandler(js["msgid"].get<int>());
        // 回调消息绑定好的事件处理器，来执行相应的业务处理
        json responsejs = msgHandler(conn, js);

        if (request->msgType == CLIENT_MSG) {
            response.msgType = request->msgType;
            response.protocolData = responsejs.dump();
            std::unique_lock<std::mutex> lock(cipherMutex_);
            response.protocolData = userCipherMap_[conn]->encrypt(response.protocolData);
            lock.unlock();
            conn->getCodec()->encode(conn->getOutBuffer(), &response);
        }
        return;
    }
    else if (request->msgType == HEARTBEAT) {
        response.msgType = HEARTBEAT;
        conn->getCodec()->encode(conn->getOutBuffer(), &response);
        return;
    }
}

// 获取消息对应的处理器
MsgHandler ChatServiceDispatcher::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end()) {
        // 返回一个默认的处理器，空操作
        return [=](const corpc::TcpConnection::ptr &conn, json &js) -> json {
            json responsejs;
            USER_LOG_ERROR << "msgid: " << msgid << " cannot find handler!";
            return responsejs;
        };
    }
    return msgHandlerMap_[msgid];
}

// 处理登录业务
json ChatServiceDispatcher::login(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do login service!!!";

    int id = js["id"].get<int>();

    LoginRequest loginReq;
    LoginResponse loginRes;
    loginReq.set_user_id(id);
    loginReq.set_user_password(js["password"]);
    loginReq.set_auth_info(corpc::getServer()->getLocalAddr()->toString());
    doLoginReq(loginReq, loginRes);

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

        // 查询用户信息
        UserInfoRequest userInfoReq;
        UserInfoResponse userInfoRes;
        userInfoReq.set_user_id(id);
        int ret = doGetUserInfoReq(userInfoReq, userInfoRes);
        if (userInfoRes.ret_code() == ProxyService::SUCCESS) {
            response["id"] = id;
            response["name"] = userInfoRes.user().name();
            std::string state = UserState_descriptor()->FindValueByNumber(userInfoRes.user().state())->name();
            std::transform(state.begin(), state.end(), state.begin(), tolower);
            response["state"] = state;
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
                std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                std::transform(state.begin(), state.end(), state.begin(), tolower);
                js["state"] = state;
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
                    std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                    std::transform(state.begin(), state.end(), state.begin(), tolower);
                    js["state"] = state;
                    std::string role = GroupUserRole_descriptor()->FindValueByNumber(user.role())->name();
                    std::transform(role.begin(), role.end(), role.begin(), tolower);
                    js["role"] = role;
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

    return response;
}

// 处理注册业务
json ChatServiceDispatcher::reg(const corpc::TcpConnection::ptr &conn, json &js)
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

    return response;
}

json ChatServiceDispatcher::oneChat(const corpc::TcpConnection::ptr &conn, json &js)
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
            // lock.unlock();
            ChatServiceStruct resp;
            resp.msgType = CLIENT_MSG;
            resp.protocolData = js.dump();
            {
                std::unique_lock<std::mutex> lock2(cipherMutex_);
                resp.protocolData = userCipherMap_[it->second]->encrypt(resp.protocolData);
            }
            it->second->getCodec()->encode(it->second->getOutBuffer(), &resp);
            it->second->output();

            response["errno"] = ProxyService::SUCCESS;
            response["errmsg"] = ProxyService::getErrorMsg(SUCCESS);
            return response;
        }
    }

    OneChatRequest oneChatReq;
    OneChatResponse oneChatRes;
    oneChatReq.set_from_user_id(fromid);
    oneChatReq.set_to_user_id(toid);
    oneChatReq.set_msg(js.dump());
    doOneChatReq(oneChatReq, oneChatRes);
    response["errno"] = oneChatRes.ret_code();
    response["errmsg"] = oneChatRes.res_info();

    return response;
}

// 添加好友业务
json ChatServiceDispatcher::addFriend(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do add friend service!!!";
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    AddFriendRequest addFriendReq;
    AddFriendResponse addFriendRes;
    addFriendReq.set_user_id(userid);
    addFriendReq.set_friend_id(friendid);
    doAddFriendReq(addFriendReq, addFriendRes);
    
    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;
    response["errno"] = addFriendRes.ret_code();
    response["errmsg"] = addFriendRes.res_info();
    // 存储好友信息
    if (addFriendRes.ret_code() == ProxyService::SUCCESS) {
        FriendListRequest friendListReq;
        FriendListResponse friendListRes;
        friendListReq.set_user_id(userid);
        doGetFriendListReq(friendListReq, friendListRes);
        if (friendListRes.ret_code() == ProxyService::SUCCESS) {
            std::vector<json> vec2;
            for (auto &user : friendListRes.friends()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                std::transform(state.begin(), state.end(), state.begin(), tolower);
                js["state"] = state;
                vec2.push_back(js);
            }
            response["friends"] = vec2;
        }
        else {
            response["errno"] = friendListRes.ret_code();
            response["errmsg"] = friendListRes.res_info();
        }
    }

    return response;
}

// 删除好友业务
json ChatServiceDispatcher::deleteFriend(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do delete friend service!!!";
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    DeleteFriendRequest delFriendReq;
    DeleteFriendResponse delFriendRes;
    delFriendReq.set_user_id(userid);
    delFriendReq.set_friend_id(friendid);
    doDeleteFriendReq(delFriendReq, delFriendRes);
    
    json response;
    response["msgid"] = DEL_FRIEND_MSG_ACK;
    response["errno"] = delFriendRes.ret_code();
    response["errmsg"] = delFriendRes.res_info();
    if (delFriendRes.ret_code() == ProxyService::SUCCESS) {
        FriendListRequest friendListReq;
        FriendListResponse friendListRes;
        friendListReq.set_user_id(userid);
        doGetFriendListReq(friendListReq, friendListRes);
        if (friendListRes.ret_code() == ProxyService::SUCCESS) {
            std::vector<json> vec2;
            for (auto &user : friendListRes.friends()) {
                json js;
                js["id"] = user.id();
                js["name"] = user.name();
                std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                std::transform(state.begin(), state.end(), state.begin(), tolower);
                js["state"] = state;
                vec2.push_back(js);
            }
            response["friends"] = vec2;
        }
        else {
            response["errno"] = friendListRes.ret_code();
            response["errmsg"] = friendListRes.res_info();
        }
    }

    return response;
}

// 处理客户端异常退出
void ChatServiceDispatcher::clientCloseException(const corpc::TcpConnection::ptr &conn)
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
                {
                    std::unique_lock<std::mutex> lock2(cipherMutex_);
                    userCipherMap_.erase(userCipherMap_.find(conn));
                }
                break;
            }
        }
    }

    // 用户注销，相当于下线
    LogoutRequest logoutReq;
    LogoutResponse logoutRes;
    logoutReq.set_user_id(userid);
    doLogoutReq(logoutReq, logoutRes);
}

// 服务器异常，业务重置方法
void ChatServiceDispatcher::reset()
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
        doLogoutReq(logoutReq, logoutRes);
    }
    userConnMap_.clear();
    std::unique_lock<std::mutex> lock2(cipherMutex_);
    userCipherMap_.clear();
}

// 创建群组业务
json ChatServiceDispatcher::createGroup(const corpc::TcpConnection::ptr &conn, json &js)
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
    doCreateGroupReq(createGroupReq, createGroupRes);

    json response;
    response["msgid"] = CREATE_GROUP_MSG_ACK;
    response["errno"] = createGroupRes.ret_code();
    response["errmsg"] = createGroupRes.res_info();
    if (createGroupRes.ret_code() == ProxyService::SUCCESS) {
        GetUserGroupsRequest userGroupsReq;
        GetUserGroupsResponse userGroupsRes;
        userGroupsReq.set_user_id(userid);
        doGetUserGroupsReq(userGroupsReq, userGroupsRes);
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
                    std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                    std::transform(state.begin(), state.end(), state.begin(), tolower);
                    js["state"] = state;
                    std::string role = GroupUserRole_descriptor()->FindValueByNumber(user.role())->name();
                    std::transform(role.begin(), role.end(), role.begin(), tolower);
                    js["role"] = role;
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

    return response;
}

// 加入群组业务
json ChatServiceDispatcher::addGroup(const corpc::TcpConnection::ptr &conn, json &js)
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
        GetUserGroupsRequest userGroupsReq;
        GetUserGroupsResponse userGroupsRes;
        userGroupsReq.set_user_id(userid);
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
                    std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                    std::transform(state.begin(), state.end(), state.begin(), tolower);
                    js["state"] = state;
                    std::string role = GroupUserRole_descriptor()->FindValueByNumber(user.role())->name();
                    std::transform(role.begin(), role.end(), role.begin(), tolower);
                    js["role"] = role;
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

    return response;
}

// 退出群组业务
json ChatServiceDispatcher::quitGroup(const corpc::TcpConnection::ptr &conn, json &js)
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
        GetUserGroupsRequest userGroupsReq;
        GetUserGroupsResponse userGroupsRes;
        userGroupsReq.set_user_id(userid);
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
                    std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                    std::transform(state.begin(), state.end(), state.begin(), tolower);
                    js["state"] = state;
                    std::string role = GroupUserRole_descriptor()->FindValueByNumber(user.role())->name();
                    std::transform(role.begin(), role.end(), role.begin(), tolower);
                    js["role"] = role;
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

    return response;
}

// 群组聊天业务
json ChatServiceDispatcher::groupChat(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do group chat service!!!";
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    GroupChatRequest groupChatReq;
    GroupChatResponse groupChatRes;
    groupChatReq.set_from_user_id(userid);
    groupChatReq.set_to_group_id(groupid);
    groupChatReq.set_msg(js.dump());
    doGroupChatReq(groupChatReq, groupChatRes);

    json response;
    response["msgid"] = GROUP_CHAT_MSG_ACK;
    response["errno"] = groupChatRes.ret_code();
    response["errmsg"] = groupChatRes.res_info();

    return response;
}

// 处理注销业务
json ChatServiceDispatcher::logout(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do logout service!!!";
    int userid = js["id"].get<int>();

    LogoutRequest logoutReq;
    LogoutResponse logoutRes;
    logoutReq.set_user_id(userid);
    doLogoutReq(logoutReq, logoutRes);

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

    // userCipherMap_.erase(userCipherMap_.find(conn)); // fix: 客户端注销，不应该删掉这个
    return response;
}

// 处理转发过来的消息
json ChatServiceDispatcher::forwarded(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do forwarded service!!!";
    json response;
    int toid = js["to"].get<int>();
    std::string msg = js["msg"].get<std::string>();
    {
        // 临界区
        std::unique_lock<std::mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) {
            // lock.unlock();
            ChatServiceStruct resp;
            resp.msgType = CLIENT_MSG;
            resp.protocolData = msg;
            {
                std::unique_lock<std::mutex> lock2(cipherMutex_);
                resp.protocolData = userCipherMap_[it->second]->encrypt(resp.protocolData);
            }
            it->second->getCodec()->encode(it->second->getOutBuffer(), &resp);
            it->second->output();
            return response;
        }
    }
    // 转发过来后，该用户下线了，也要存离线消息
    SaveOfflineMessageRequest saveOfflineMessageReq;
    SaveOfflineMessageResponse saveOfflineMessageRes;
    saveOfflineMessageReq.set_user_id(toid);
    saveOfflineMessageReq.set_msg(msg);
    doSaveOfflineMessageReq(saveOfflineMessageReq, saveOfflineMessageRes);

    return response;
}

// 获取当前用户信息业务
json ChatServiceDispatcher::getUserInfo(const corpc::TcpConnection::ptr &conn, json &js)
{
    USER_LOG_INFO << "do get user info service!!!";
    int userid = js["id"].get<int>();

    json response;
    response["msgid"] = GET_USER_INFO_MSG_ACK;

    // 查询用户信息
    UserInfoRequest userInfoReq;
    UserInfoResponse userInfoRes;
    userInfoReq.set_user_id(userid);
    int ret = doGetUserInfoReq(userInfoReq, userInfoRes);
    if (userInfoRes.ret_code() == ProxyService::SUCCESS) {
        response["id"] = userid;
        response["name"] = userInfoRes.user().name();
        std::string state = UserState_descriptor()->FindValueByNumber(userInfoRes.user().state())->name();
        std::transform(state.begin(), state.end(), state.begin(), tolower);
        response["state"] = state;
        response["errno"] = userInfoRes.ret_code();
        response["errmsg"] = userInfoRes.res_info();
    }
    else {
        response["errno"] = userInfoRes.ret_code();
        response["errmsg"] = userInfoRes.res_info();
        return response;
    }

    // 查询该用户的好友信息并返回
    FriendListRequest friendListReq;
    FriendListResponse friendListRes;
    friendListReq.set_user_id(userid);
    ret = doGetFriendListReq(friendListReq, friendListRes);
    if (friendListRes.ret_code() == ProxyService::SUCCESS) {
        std::vector<json> vec2;
        for (auto &user : friendListRes.friends()) {
            json js;
            js["id"] = user.id();
            js["name"] = user.name();
            std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
            std::transform(state.begin(), state.end(), state.begin(), tolower);
            js["state"] = state;
            vec2.push_back(js);
        }
        response["friends"] = vec2;
    }
    else {
        response["errno"] = friendListRes.ret_code();
        response["errmsg"] = friendListRes.res_info();
        return response;
    }

    // 查询用户的群组信息
    GetUserGroupsRequest userGroupsReq;
    GetUserGroupsResponse userGroupsRes;
    userGroupsReq.set_user_id(userid);
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
                std::string state = UserState_descriptor()->FindValueByNumber(user.state())->name();
                std::transform(state.begin(), state.end(), state.begin(), tolower);
                js["state"] = state;
                std::string role = GroupUserRole_descriptor()->FindValueByNumber(user.role())->name();
                std::transform(role.begin(), role.end(), role.begin(), tolower);
                js["role"] = role;
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

    return response;
}

int ChatServiceDispatcher::doLoginReq(const ::LoginRequest &request, ::LoginResponse &response)
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

int ChatServiceDispatcher::doLogoutReq(const ::LogoutRequest &request, ::LogoutResponse &response)
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

int ChatServiceDispatcher::doRegisterReq(const ::RegisterRequest &request, ::RegisterResponse &response)
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

int ChatServiceDispatcher::doAddFriendReq(const ::AddFriendRequest &request, ::AddFriendResponse &response)
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

int ChatServiceDispatcher::doDeleteFriendReq(::DeleteFriendRequest &request, ::DeleteFriendResponse &response)
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

int ChatServiceDispatcher::doGetUserInfoReq(const ::UserInfoRequest &request, ::UserInfoResponse &response)
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

int ChatServiceDispatcher::doGetFriendListReq(const ::FriendListRequest &request, ::FriendListResponse &response)
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

int ChatServiceDispatcher::doCreateGroupReq(const ::CreateGroupRequest &request, ::CreateGroupResponse &response)
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

int ChatServiceDispatcher::doAddGroupReq(const ::AddGroupRequest &request, ::AddGroupResponse &response)
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

int ChatServiceDispatcher::doQuitGroupReq(const ::QuitGroupRequest &request, ::QuitGroupResponse &response)
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

int ChatServiceDispatcher::doGetGroupInfoReq(const ::GetGroupInfoRequest &request, ::GetGroupInfoResponse &response)
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

int ChatServiceDispatcher::doGetUserGroupsReq(const ::GetUserGroupsRequest &request, ::GetUserGroupsResponse &response)
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

int ChatServiceDispatcher::doOneChatReq(const ::OneChatRequest &request, ::OneChatResponse &response)
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

int ChatServiceDispatcher::doGroupChatReq(const ::GroupChatRequest &request, ::GroupChatResponse &response)
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

int ChatServiceDispatcher::doReadOfflineMessageReq(const ::ReadOfflineMessageRequest &request, ::ReadOfflineMessageResponse &response)
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

int ChatServiceDispatcher::doSaveOfflineMessageReq(const ::SaveOfflineMessageRequest &request, ::SaveOfflineMessageResponse &response)
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