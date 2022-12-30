/*************************************************************
 * test_get_group_users_client.cpp
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-30 10:15:13
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
 *************************************************************/

#include <google/protobuf/service.h>
#include <corpc/net/pb/pb_rpc_client_block_channel.h>
#include <corpc/net/pb/pb_rpc_controller.h>
#include <corpc/net/pb/pb_rpc_closure.h>
#include <corpc/net/net_address.h>
#include <corpc/common/start.h>
#include "GroupService/pb/GroupService.pb.h"

void testClient()
{
    corpc::IPAddress::ptr addr = std::make_shared<corpc::IPAddress>("0.0.0.0", 39999);

    corpc::PbRpcClientBlockChannel channel(addr);
    GroupServiceRpc_Stub stub(&channel);

    corpc::PbRpcController rpcController;
    rpcController.SetTimeout(5000);

    ::GetGroupUsersRequest rpcReq;
    ::GetGroupUsersResponse rpcRes;

    std::cout << "Send to GroupService server " << addr->toString() << ", request body: " << rpcReq.ShortDebugString() << std::endl;
    stub.GetGroupUsers(&rpcController, &rpcReq, &rpcRes, NULL);

    if (rpcController.ErrorCode() != 0) {
        std::cout << "Failed to call GroupService server, error code: " << rpcController.ErrorCode() << ", error info: " << rpcController.ErrorText() << std::endl;
        return;
    }

    std::cout << "Success get response from GroupService server " << addr->toString() << ", response body: " << rpcRes.ShortDebugString() << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Start GroupService test client error, input argc is not 2!");
        printf("Start GroupService like this: \n");
        printf("./test_get_group_users_client GroupService.yml\n");
        return 0;
    }

    corpc::initConfig(argv[1]);

    testClient();

    return 0;
}