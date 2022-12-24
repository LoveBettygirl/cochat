/*************************************************************
 * delete_friend.h
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-24 08:41:49
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#ifndef FRIENDSERVICE_INTERFACE_DELETE_FRIEND_H
#define FRIENDSERVICE_INTERFACE_DELETE_FRIEND_H 

#include "FriendService/pb/FriendService.pb.h"


namespace FriendService {

/**
 * Rpc Interface Class
 * Alloc one object every time RPC call begin, and destroy this object while RPC call end
*/

class DeleteFriendInterface {
public:
    DeleteFriendInterface(const ::DeleteFriendRequest& request, ::DeleteFriendResponse& response);
    ~DeleteFriendInterface();

    void run(); // Business implementation

private:
    const ::DeleteFriendRequest& request_;      // request object from client
    ::DeleteFriendResponse& response_;          // response object that reply to client
};

}

#endif