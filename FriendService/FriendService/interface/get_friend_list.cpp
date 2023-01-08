/*************************************************************
 * get_friend_list.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-24 08:41:49
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "FriendService/interface/get_friend_list.h"
#include "FriendService/pb/FriendService.pb.h"
#include "FriendService/dao/friend_dao.h"
#include "FriendService/common/business_exception.h"
#include "FriendService/common/error_code.h"
#include "FriendService/common/const.h"
#include <vector>


namespace FriendService {

GetFriendListInterface::GetFriendListInterface(const ::FriendListRequest &request, ::FriendListResponse &response)
    : request_(request), 
    response_(response)
{

}

GetFriendListInterface::~GetFriendListInterface()
{

}

void GetFriendListInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //
    int userid = request_.user_id();
    // 获取好友列表信息
    FriendDao dao;
    std::vector<User> users = dao.queryFriendList(userid);
    for (const auto &user : users) {
        ::UserInfo *info = response_.add_friends();
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

}