#ifndef PROXYSERVICE_CHAT_SERVICE_DISPATCHER_H
#define PROXYSERVICE_CHAT_SERVICE_DISPATCHER_H

#include <string>
#include <corpc/net/custom/custom_data.h>
#include <corpc/net/custom/custom_dispatcher.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <corpc/net/tcp/tcp_connection.h>
#include "ProxyService/lib/json.hpp"
#include "ProxyService/pb/ProxyService.pb.h"
#include "ProxyService/cypher/rsa.h"
#include "ProxyService/cypher/aes.h"
#include <corpc/net/service_register.h>
#include "ProxyService/common/const.h"

namespace ProxyService {

using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<json (const corpc::TcpConnection::ptr &conn, json &js)>;

class ChatServiceDispatcher : public corpc::CustomDispatcher {
public:
    ChatServiceDispatcher();
    ~ChatServiceDispatcher() {}

    void dispatch(corpc::AbstractData *data, const corpc::TcpConnection::ptr &conn) override;

    // 处理客户端异常退出
    void clientCloseException(const corpc::TcpConnection::ptr &conn);

    // 服务器异常，业务重置方法
    void reset();

private:
    // 存储消息id和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接
    std::unordered_map<int, corpc::TcpConnection::ptr> userConnMap_;

    // 定义互斥锁，保证_userConnMap的线程安全
    std::mutex connMutex_;

    std::string rsaPrivateKey_, rsaPublicKey_;

    RSATool::ptr rsa_;

    std::unordered_map<corpc::TcpConnection::ptr, AESTool::ptr> userCipherMap_;

    std::mutex cipherMutex_;

    corpc::AbstractServiceRegister::ptr center_;

private:
    // 处理登录业务
    json login(const corpc::TcpConnection::ptr &conn, json &js);

    // 处理注册业务
    json reg(const corpc::TcpConnection::ptr &conn, json &js);

    // 一对一聊天业务
    json oneChat(const corpc::TcpConnection::ptr &conn, json &js);

    // 添加好友业务
    json addFriend(const corpc::TcpConnection::ptr &conn, json &js);

    // 删除好友业务
    json deleteFriend(const corpc::TcpConnection::ptr &conn, json &js);

    // 创建群组业务
    json createGroup(const corpc::TcpConnection::ptr &conn, json &js);

    // 加入群组业务
    json addGroup(const corpc::TcpConnection::ptr &conn, json &js);

    // 退出群组业务
    json quitGroup(const corpc::TcpConnection::ptr &conn, json &js);

    // 群组聊天业务
    json groupChat(const corpc::TcpConnection::ptr &conn, json &js);

    // 处理注销业务
    json logout(const corpc::TcpConnection::ptr &conn, json &js);

    // 处理转发过来的消息
    json forwarded(const corpc::TcpConnection::ptr &conn, json &js);

    // 获取当前用户信息业务
    json getUserInfo(const corpc::TcpConnection::ptr &conn, json &js);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    int doLoginReq(const ::LoginRequest &request, ::LoginResponse &response);
    int doLogoutReq(const ::LogoutRequest &request, ::LogoutResponse &response);
    int doRegisterReq(const ::RegisterRequest &request, ::RegisterResponse &response);

    int doAddFriendReq(const ::AddFriendRequest &request, ::AddFriendResponse &response);
    int doDeleteFriendReq(::DeleteFriendRequest &request, ::DeleteFriendResponse &response);
    int doGetUserInfoReq(const ::UserInfoRequest &request, ::UserInfoResponse &response);
    int doGetFriendListReq(const ::FriendListRequest &request, ::FriendListResponse &response);

    int doCreateGroupReq(const ::CreateGroupRequest &request, ::CreateGroupResponse &response);
    int doAddGroupReq(const ::AddGroupRequest &request, ::AddGroupResponse &response);
    int doQuitGroupReq(const ::QuitGroupRequest &request, ::QuitGroupResponse &response);
    int doGetGroupInfoReq(const ::GetGroupInfoRequest &request, ::GetGroupInfoResponse &response);
    int doGetUserGroupsReq(const ::GetUserGroupsRequest &request, ::GetUserGroupsResponse &response);

    int doOneChatReq(const ::OneChatRequest &request, ::OneChatResponse &response);
    int doGroupChatReq(const ::GroupChatRequest &request, ::GroupChatResponse &response);
    int doReadOfflineMessageReq(const ::ReadOfflineMessageRequest &request, ::ReadOfflineMessageResponse &response);
    int doSaveOfflineMessageReq(const ::SaveOfflineMessageRequest &request, ::SaveOfflineMessageResponse &response);

    void readRsaKey(const std::string &fileName, std::string &pemFile);
    void sendMsg(const corpc::TcpConnection::ptr &conn, int pkType, const std::string &data);
    void sendMsgInCor(const corpc::TcpConnection::ptr &conn, int pkType, const std::string &data);
};

}

#endif