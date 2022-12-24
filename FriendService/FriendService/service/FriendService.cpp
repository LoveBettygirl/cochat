/*************************************************************
 * FriendService.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-24 08:41:49
 * This file will be overwrite every time
 *************************************************************/

#include <google/protobuf/service.h>
#include <exception>
#include <corpc/common/log.h>
#include "FriendService/pb/FriendService.pb.h"
#include "FriendService/service/FriendService.h"
#include "FriendService/common/business_exception.h"
#include "FriendService/interface/get_friend_list.h"
#include "FriendService/interface/get_user_info.h"
#include "FriendService/interface/add_friend.h"
#include "FriendService/interface/delete_friend.h"


#define CALL_RPC_INTERFACE(type)                                                                               \
    type impl(*request, *response);                                                                            \
    try {                                                                                                      \
        USER_LOG_INFO << "In|request:{" << request->ShortDebugString() << "}";                                 \
        impl.run();                                                                                            \
        response->set_ret_code(0);                                                                             \
        response->set_res_info("OK");                                                                          \
        USER_LOG_INFO << "Out|response:{" << response->ShortDebugString() << "}";                              \
        if (done) {                                                                                            \
            done->Run();                                                                                       \
        }                                                                                                      \
    }                                                                                                          \
    catch (FriendService::BusinessException &e) {                                                            \
        USER_LOG_ERROR << "[" << e.fileName() << ":" << e.line() << "] occur BusinessException, error code = " \
                       << e.code() << ", errinfo = " << e.error();                                             \
        response->set_ret_code(e.code());                                                                      \
        response->set_res_info(e.error());                                                                     \
        USER_LOG_INFO << "Out|response:{" << response->ShortDebugString() << "}";                              \
        if (done) {                                                                                            \
            done->Run();                                                                                       \
        }                                                                                                      \
    }                                                                                                          \
    catch (std::exception &) {                                                                                 \
        USER_LOG_ERROR << "occur std::exception, error code = -1, errorinfo = Unknown error ";                 \
        response->set_ret_code(-1);                                                                            \
        response->set_res_info("Unknown error");                                                               \
        USER_LOG_INFO << "Out|response:{" << response->ShortDebugString() << "}";                              \
        if (done) {                                                                                            \
            done->Run();                                                                                       \
        }                                                                                                      \
    }                                                                                                          \
    catch (...) {                                                                                              \
        USER_LOG_ERROR << "occur Unknown exception, error code = -1, errorinfo = Unknown error ";              \
        response->set_ret_code(-1);                                                                            \
        response->set_res_info("Unknown error");                                                               \
        USER_LOG_INFO << "Out|response:{" << response->ShortDebugString() << "}";                              \
        if (done) {                                                                                            \
            done->Run();                                                                                       \
        }                                                                                                      \
    }                                                                                                          \
    if (done) {                                                                                                \
        done->Run();                                                                                           \
    }


namespace FriendService {

void FriendServiceRpcImpl::GetFriendList(::google::protobuf::RpcController* controller,
                       const ::FriendListRequest* request,
                       ::FriendListResponse* response,
                       ::google::protobuf::Closure* done)
{
    CALL_RPC_INTERFACE(GetFriendListInterface);
}

void FriendServiceRpcImpl::GetUserInfo(::google::protobuf::RpcController* controller,
                       const ::UserInfoRequest* request,
                       ::UserInfoResponse* response,
                       ::google::protobuf::Closure* done)
{
    CALL_RPC_INTERFACE(GetUserInfoInterface);
}

void FriendServiceRpcImpl::AddFriend(::google::protobuf::RpcController* controller,
                       const ::AddFriendRequest* request,
                       ::AddFriendResponse* response,
                       ::google::protobuf::Closure* done)
{
    CALL_RPC_INTERFACE(AddFriendInterface);
}

void FriendServiceRpcImpl::DeleteFriend(::google::protobuf::RpcController* controller,
                       const ::DeleteFriendRequest* request,
                       ::DeleteFriendResponse* response,
                       ::google::protobuf::Closure* done)
{
    CALL_RPC_INTERFACE(DeleteFriendInterface);
}

}