/*************************************************************
 * get_user_info.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-24 08:41:49
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "FriendService/interface/get_user_info.h"
#include "FriendService/pb/FriendService.pb.h"
#include "FriendService/dao/user_dao.h"
#include "FriendService/common/business_exception.h"
#include "FriendService/common/error_code.h"
#include "FriendService/common/const.h"


namespace FriendService {

GetUserInfoInterface::GetUserInfoInterface(const ::UserInfoRequest &request, ::UserInfoResponse &response)
    : request_(request), 
    response_(response)
{

}

GetUserInfoInterface::~GetUserInfoInterface()
{

}

void GetUserInfoInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //
    int userid = request_.user_id();

    UserDao dao;
    User user = dao.queryUserInfo(userid);
    if (user.getState() == NOT_EXIST_STATE) {
        throw BusinessException(CURRENT_USER_NOT_EXIST, getErrorMsg(CURRENT_USER_NOT_EXIST), __FILE__, __LINE__);
    }
    ::UserInfo *info = response_.mutable_user();
    info->set_id(user.getId());
    info->set_name(user.getName());

    if (user.getState() == ONLINE_STATE) {
        info->set_state(::UserState::ONLINE);
    }
    else if (user.getState() == OFFLINE_STATE) {
        info->set_state(::UserState::OFFLINE);
    }
}

}