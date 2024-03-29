/*************************************************************
 * save_offline_message.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2023-01-16 11:02:51
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "ChatService/interface/save_offline_message.h"
#include "ChatService/pb/ChatService.pb.h"
#include "ChatService/lib/rocketmq/rocketmq.h"
#include "ChatService/dao/user_dao.h"
#include "ChatService/dao/offline_message_dao.h"
#include "ChatService/common/business_exception.h"
#include "ChatService/common/const.h"
#include "ChatService/common/error_code.h"
#include <string>


namespace ChatService {

extern RocketMQProducer::ptr gProducer;

SaveOfflineMessageInterface::SaveOfflineMessageInterface(const ::SaveOfflineMessageRequest &request, ::SaveOfflineMessageResponse &response)
    : request_(request), 
    response_(response)
{

}

SaveOfflineMessageInterface::~SaveOfflineMessageInterface()
{

}

void SaveOfflineMessageInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //
    int userid = request_.user_id();
    std::string msg = request_.msg();

    UserDao userDao;
    User user = userDao.queryUserState(userid);
    if (user.getState() == NOT_EXIST_STATE) {
        throw BusinessException(CURRENT_USER_NOT_EXIST, getErrorMsg(CURRENT_USER_NOT_EXIST), __FILE__, __LINE__);
    }

    // 异步存储离线消息
    if (!gProducer->send(SAVE_OFFLINE_MSG_TOPIC, std::to_string(userid), msg)) {
        throw BusinessException(SAVE_OFFLINE_MSG_FAILED, getErrorMsg(SAVE_OFFLINE_MSG_FAILED), __FILE__, __LINE__);
    } 
}

}