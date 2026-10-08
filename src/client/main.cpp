#include "StunClient.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
    #ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    #endif

    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);

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

        std::thread net_thread([&io_context](){
            io_context.run();
        });
        client.runUdpchat();
        
        work_guard.reset();
        io_context.stop();

        if (net_thread.joinable()) 
        {
            net_thread.join();
        }
    }
    else if (argc == 2) // port
    {
        auto endpoints = resolver.resolve(udp::v4(), ip, port);
        udp::endpoint targetEndpoint = *endpoints.begin();
        StunClient client(io_context, atoi(argv[1]), targetEndpoint);

        std::thread net_thread([&io_context](){
            io_context.run();
        });
        client.runUdpchat();
        
        work_guard.reset();
        io_context.stop();

        if (net_thread.joinable()) 
        {
            net_thread.join();
        }

    }
    else
    {

        printf("usage: %s local_port or %s", argv[0]);
    }
    
}