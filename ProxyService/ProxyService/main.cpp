#include <iostream>
#include <string>
#include <signal.h>
#include <corpc/common/start.h>
#include "ProxyService/chat_service.h"

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    std::shared_ptr<ProxyService::ChatServiceDispatcher> dispatcher = std::dynamic_pointer_cast<ProxyService::ChatServiceDispatcher>(corpc::getServer()->getDispatcher());
    if (dispatcher) {
        dispatcher->reset();
    }
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

    REGISTER_SERVICE(ProxyService::ChatService);

    corpc::startServer();

    return 0;
}