#include "StunClient.h"

#ifdef _WIN32
#include <windows.h>
#endif


int main(int argc, char* argv[])
{
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // 控制台输出用 UTF-8
    SetConsoleCP(CP_UTF8);         // 控制台输入用 UTF-8
    #endif

    boost::asio::io_context io_context;
    udp::resolver resolver(io_context);

    std::string ip;
    std::string port("3478");

    std::cout << "[Server Ip]:" << std::flush;
    std::getline(std::cin, ip);

    if (argc == 1) //stunclient
    {
        auto endpoints = resolver.resolve(udp::v4(), ip, port);
        udp::endpoint targetEndpoint = *endpoints.begin();
        StunClient client(io_context, 0, targetEndpoint);

        std::thread input_thread([&client](){
            client.runUdpchat();
        });
        io_context.run();

        if (input_thread.joinable()) 
        {
            input_thread.join();
        }
    }
    else if (argc == 2) // port
    {
        auto endpoints = resolver.resolve(udp::v4(), ip, port);
        udp::endpoint targetEndpoint = *endpoints.begin();
        StunClient client(io_context, atoi(argv[1]), targetEndpoint);

        std::thread input_thread([&client](){
            client.runUdpchat();
        });
        io_context.run();

        if (input_thread.joinable()) 
        {
            input_thread.join();
        }

    }
    else
    {

        printf("usage: %s local_port or %s", argv[0]);
    }
    
}