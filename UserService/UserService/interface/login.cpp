/*************************************************************
 * login.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-17 11:24:33
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "UserService/interface/login.h"
#include "UserService/pb/UserService.pb.h"
#include "UserService/dao/user_dao.h"
#include "UserService/common/business_exception.h"
#include "UserService/common/error_code.h"


namespace UserService {

LoginInterface::LoginInterface(const ::LoginRequest &request, ::LoginResponse &response)
    : request_(request), 
    response_(response)
{

}

LoginInterface::~LoginInterface()
{

}

void LoginInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //
    int id = request_.id();
    std::string pwd = request_.password();

    UserDao dao;
    User user = dao.queryInfo(id);
    if (user.getId() == id && user.getPwd() == pwd) {
        if (user.getState() == "online") {
            // 该用户已经登录，不允许重复登录
            dao.updateState(user);
            throw BusinessException(ACCOUNT_LOGGED_IN, getErrorMsg(ACCOUNT_LOGGED_IN), __FILE__, __LINE__);
        }
        else {
            // 登录成功，更新用户状态信息offline -> online
            user.setState("online");
            dao.updateState(user);
        }
    }
    else {
        // 该用户不存在，登录失败
        if (user.getState() != "not_exist") {
            dao.updateState(user);
        }
        throw BusinessException(INVALID_ID_OR_PWD, getErrorMsg(INVALID_ID_OR_PWD), __FILE__, __LINE__);
    }
}

}