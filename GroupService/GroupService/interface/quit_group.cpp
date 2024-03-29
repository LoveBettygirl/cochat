/*************************************************************
 * quit_group.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-30 10:15:13
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "GroupService/interface/quit_group.h"
#include "GroupService/pb/GroupService.pb.h"
#include "GroupService/dao/group_dao.h"
#include "GroupService/common/business_exception.h"
#include "GroupService/common/const.h"
#include "GroupService/common/error_code.h"
#include "GroupService/dao/user_dao.h"


namespace GroupService {

QuitGroupInterface::QuitGroupInterface(const ::QuitGroupRequest &request, ::QuitGroupResponse &response)
    : request_(request), 
    response_(response)
{

}

QuitGroupInterface::~QuitGroupInterface()
{

}

void QuitGroupInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //
    int userid = request_.user_id();
    int groupid = request_.group_id();

    UserDao userDao;
    User user = userDao.queryUserInfo(userid);
    if (user.getState() == NOT_EXIST_STATE) {
        throw BusinessException(CURRENT_USER_NOT_EXIST, getErrorMsg(CURRENT_USER_NOT_EXIST), __FILE__, __LINE__);
    }

    GroupDao groupDao;
    Group group = groupDao.queryGroup(groupid);
    // 群组是否存在
    if (group.getId() == -1) {
        throw BusinessException(GROUP_NOT_EXIST, getErrorMsg(GROUP_NOT_EXIST), __FILE__, __LINE__);
    }

    std::string role = groupDao.queryGroupUserRole(userid, groupid);
    if (role.empty()) {
        throw BusinessException(USER_NOT_IN_GROUP, getErrorMsg(USER_NOT_IN_GROUP), __FILE__, __LINE__);
    }
    if (role == CREATOR_ROLE) {
        throw BusinessException(USER_IS_GROUP_CREATOR, getErrorMsg(USER_IS_GROUP_CREATOR), __FILE__, __LINE__);
    }
    if (!groupDao.quitGroup(userid, groupid)) {
        throw BusinessException(USER_NOT_IN_GROUP, getErrorMsg(USER_NOT_IN_GROUP), __FILE__, __LINE__);
    }
}

}