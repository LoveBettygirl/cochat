/*************************************************************
 * GroupService.h
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2022-12-30 10:15:13
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/


#include <google/protobuf/service.h>
#include <corpc/common/start.h>
#include "GroupService/service/GroupService.h"


int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Start GroupService server error, input argc is not 2!");
        printf("Start GroupService like this: \n");
        printf("./GroupService GroupService.yml\n");
        return 0;
    }

    corpc::initConfig(argv[1]);

    REGISTER_SERVICE(GroupService::GroupServiceRpcImpl);

    corpc::startServer();
    
    return 0;
}