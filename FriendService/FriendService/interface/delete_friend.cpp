/*************************************************************
 * delete_friend.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-24 08:41:49
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "FriendService/interface/delete_friend.h"
#include "FriendService/pb/FriendService.pb.h"
#include "FriendService/dao/friend_dao.h"
#include "FriendService/dao/user_dao.h"
#include "FriendService/common/business_exception.h"
#include "FriendService/common/error_code.h"


namespace FriendService {

DeleteFriendInterface::DeleteFriendInterface(const ::DeleteFriendRequest &request, ::DeleteFriendResponse &response)
    : request_(request), 
    response_(response)
{

}

DeleteFriendInterface::~DeleteFriendInterface()
{

}

void DeleteFriendInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //
    int userid = request_.user_id();
    int friendid = request_.friend_id();

    if (userid == friendid) {
        throw BusinessException(FRIEND_IS_SELF_USER, getErrorMsg(FRIEND_IS_SELF_USER), __FILE__, __LINE__);
    }

    UserDao userDao;
    User user = userDao.queryUserInfo(userid);
    if (user.getState() == NOT_EXIST_STATE) {
        throw BusinessException(CURRENT_USER_NOT_EXIST, getErrorMsg(CURRENT_USER_NOT_EXIST), __FILE__, __LINE__);
    }
    User friendUser = userDao.queryUserInfo(friendid);
    if (friendUser.getState() == NOT_EXIST_STATE) {
        throw BusinessException(FRIEND_USER_NOT_EXIST, getErrorMsg(FRIEND_USER_NOT_EXIST), __FILE__, __LINE__);
    }

    FriendDao friendDao;
    if (!friendDao.deleteFriend(userid, friendid)) {
        throw BusinessException(FRIEND_RELATION_NOT_EXIST, getErrorMsg(FRIEND_RELATION_NOT_EXIST), __FILE__, __LINE__);
    }
}

}