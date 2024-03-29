/*************************************************************
 * save_offline_message.h
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2023-01-16 11:02:51
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#ifndef CHATSERVICE_INTERFACE_SAVE_OFFLINE_MESSAGE_H
#define CHATSERVICE_INTERFACE_SAVE_OFFLINE_MESSAGE_H 

#include "ChatService/pb/ChatService.pb.h"


namespace ChatService {

/**
 * Rpc Interface Class
 * Alloc one object every time RPC call begin, and destroy this object while RPC call end
*/

class SaveOfflineMessageInterface {
public:
    SaveOfflineMessageInterface(const ::SaveOfflineMessageRequest& request, ::SaveOfflineMessageResponse& response);
    ~SaveOfflineMessageInterface();

    void run(); // Business implementation

private:
    const ::SaveOfflineMessageRequest& request_;      // request object from client
    ::SaveOfflineMessageResponse& response_;          // response object that reply to client
};

}

#endif