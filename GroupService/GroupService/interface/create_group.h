/*************************************************************
 * create_group.h
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-30 10:15:13
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#ifndef GROUPSERVICE_INTERFACE_CREATE_GROUP_H
#define GROUPSERVICE_INTERFACE_CREATE_GROUP_H 

#include "GroupService/pb/GroupService.pb.h"


namespace GroupService {

/**
 * Rpc Interface Class
 * Alloc one object every time RPC call begin, and destroy this object while RPC call end
*/

class CreateGroupInterface {
public:
    CreateGroupInterface(const ::CreateGroupRequest& request, ::CreateGroupResponse& response);
    ~CreateGroupInterface();

    void run(); // Business implementation

private:
    const ::CreateGroupRequest& request_;      // request object from client
    ::CreateGroupResponse& response_;          // response object that reply to client
};

}

#endif