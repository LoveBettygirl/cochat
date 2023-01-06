/*************************************************************
 * OfflineService.h
 * Generated by corpc framework corpc_gen.py
 * Create Time: 2023-01-05 09:36:18
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/


#include <google/protobuf/service.h>
#include <corpc/common/start.h>
#include "OfflineService/service/OfflineService.h"


int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Start OfflineService server error, input argc is not 2!");
        printf("Start OfflineService like this: \n");
        printf("./OfflineService OfflineService.yml\n");
        return 0;
    }

    corpc::initConfig(argv[1]);

    REGISTER_SERVICE(OfflineService::OfflineServiceRpcImpl);

    corpc::startServer();
    
    return 0;
}