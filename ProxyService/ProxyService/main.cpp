#include <iostream>
#include <string>
#include <signal.h>
#include <corpc/common/start.h>
#include "ProxyService/chat_server.h"
#include "ProxyService/chat_service.h"

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ProxyService::ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Start ProxyService server error, input argc is not 2!");
        printf("Start ProxyService like this: \n");
        printf("./ProxyService ProxyService.yml\n");
        return 0;
    }

    signal(SIGINT, resetHandler);

    corpc::initConfig(argv[1]);

    ProxyService::ChatServer::ptr server = std::make_shared<ProxyService::ChatServer>();

    corpc::startServer();

    return 0;
}