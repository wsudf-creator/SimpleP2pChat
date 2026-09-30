#include "StunClient.h"

int main(int argc, char* argv[])
{
    boost::asio::io_context io_context;
    udp::resolver resolver(io_context);

    if (argc == 3) //stunclient ip port
    {
        std::string ip(argv[1]);
        std::string port(argv[2]);

        auto endpoints = resolver.resolve(udp::v4(), ip, port);
        udp::endpoint targetEndpoint = *endpoints.begin();
        StunClient client(io_context, 8888, targetEndpoint);

        std::thread input_thread([&client](){
            client.runUdpchat();
        });
        io_context.run();

        if (input_thread.joinable()) 
        {
            input_thread.join();
        }
    }
    else if (argc == 4)
    {
        std::string ip(argv[1]);
        std::string remote_port(argv[2]);

        auto endpoints = resolver.resolve(udp::v4(), ip, remote_port);
        udp::endpoint targetEndpoint = *endpoints.begin();
        StunClient client(io_context, atoi(argv[3]), targetEndpoint);

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
        // auto endpoints = resolver.resolve(udp::v4(), "stun.12voip.com", "3478");
        // udp::endpoint targetEndpoint = *endpoints.begin();
        // StunClient client(io_context, 9999, targetEndpoint);
        // io_context.run();
        printf("usage: %s server_address server_port local_port", argv[0]);
    }
    
}