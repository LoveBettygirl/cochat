#include "TestClient/chat_client.h"
#include <iostream>
#include <regex>
#include <getopt.h>

int port = 2000;
std::string ip = "127.0.0.1";

void showUsage()
{
    std::cout << "Options:" << std::endl;
    std::cout << " -a, --address=IP_ADDRESS   The IP address of the server." << std::endl;
    std::cout << " -p, --port=PORT            The port of the server." << std::endl;
    std::cout << " -h, --help                 Print this message and exit." << std::endl;
    exit(-1);
}

void parseArgs(int argc, char *argv[])
{
    int c;
    std::regex r("^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
            "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
            "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
            "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$");

    if (argc == 1)
        showUsage();

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"address", required_argument, 0, 'a'},
            {"port", required_argument, 0, 'p'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "o:a:p:vh",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'a':
            ip = std::string(optarg);
            if (!regex_match(ip, r)) {
                std::cerr << "Invaild IP address format." << std::endl;
                exit(-1);
            }
            break;

        case 'p':
            port = atoi(optarg);
            if (port < 0 || port > 65535) {
                std::cerr << "The port " << port << " out of range." << std::endl;
                exit(-1);
            }
            break;

        case 'h':
        case '?':
            showUsage();
            break;

        default:
            showUsage();
        }
    }

    if (optind < argc)
        showUsage();
}

int main(int argc, char *argv[])
{
    parseArgs(argc, argv);

    TestClient::ChatClient client(ip, port);
    client.start();

    return 0;
}