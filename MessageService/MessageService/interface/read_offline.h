/*************************************************************
 * read_offline.h
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2023-01-10 10:20:34
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#ifndef MESSAGESERVICE_INTERFACE_READ_OFFLINE_H
#define MESSAGESERVICE_INTERFACE_READ_OFFLINE_H 

#include "MessageService/pb/MessageService.pb.h"


namespace MessageService {

/**
 * Rpc Interface Class
 * Alloc one object every time RPC call begin, and destroy this object while RPC call end
*/

class ReadOfflineInterface {
public:
    ReadOfflineInterface(const ::ReadOfflineRequest& request, ::ReadOfflineResponse& response);
    ~ReadOfflineInterface();

    void run(); // Business implementation

private:
    const ::ReadOfflineRequest& request_;      // request object from client
    ::ReadOfflineResponse& response_;          // response object that reply to client
};

}

#endif