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
#include "UserService/common/const.h"


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
    int id = request_.user_id();
    std::string pwd = request_.user_password();
    std::string host = request_.auth_info();
    corpc::NetAddress::ptr addr = std::make_shared<corpc::IPAddress>(host);

    if (pwd.empty()) {
        throw BusinessException(USER_PWD_IS_EMPTY, getErrorMsg(USER_PWD_IS_EMPTY), __FILE__, __LINE__);
    }
    if (addr->toString() == "0.0.0.0:0") {
        throw BusinessException(ILLEGAL_USER_HOST, getErrorMsg(ILLEGAL_USER_HOST), __FILE__, __LINE__);
    }

    UserDao dao;
    User user = dao.queryUserInfo(id);
    std::string salt = user.getSalt();
    std::reverse(salt.begin(), salt.end());
    std::string newPwd = pwd + salt;
    corpc::MD5 md5;
    newPwd = md5.getResultString(newPwd);
    if (user.getId() == id && user.getPwd() == newPwd) {
        if (user.getState() == ONLINE_STATE) {
            // 该用户已经登录，不允许重复登录
            dao.updateUserState(user);
            throw BusinessException(USER_LOGGED_IN, getErrorMsg(USER_LOGGED_IN), __FILE__, __LINE__);
        }
        else {
            // 登录成功，更新用户状态信息offline -> online
            user.setState(ONLINE_STATE);
            dao.updateUserState(user);
            dao.saveUserHost(id, addr);
        }
    }
    else {
        // 该用户不存在，登录失败
        if (user.getState() != NOT_EXIST_STATE) {
            dao.updateUserState(user);
            dao.removeUserHost(id);
        }
        throw BusinessException(INVALID_USER_ID_OR_PWD, getErrorMsg(INVALID_USER_ID_OR_PWD), __FILE__, __LINE__);
    }
}

}