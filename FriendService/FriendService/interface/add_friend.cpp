/*************************************************************
 * add_friend.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-24 08:41:49
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#include <corpc/common/log.h>
#include "FriendService/interface/add_friend.h"
#include "FriendService/pb/FriendService.pb.h"


namespace FriendService {

AddFriendInterface::AddFriendInterface(const ::AddFriendRequest &request, ::AddFriendResponse &response)
    : request_(request), 
    response_(response)
{

}

AddFriendInterface::~AddFriendInterface()
{

}

void AddFriendInterface::run()
{
    //
    // Run your business at here
    // response_.set_ret_code(0);
    // response_.set_res_info("Succ");
    //

}

}