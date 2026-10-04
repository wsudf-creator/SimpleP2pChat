#pragma once

#include "../utility/ConstDef.h"
#include "../utility/utility.h"

#include "Punch.h"
#include "UdpChat.h"
#include <atomic>
#include <array>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;



class StunClient
{
public:
    StunClient(boost::asio::io_context& io_context, uint16_t localPort, const std::string& remoteIp, uint16_t remotePort);
    StunClient(boost::asio::io_context& io_context, uint16_t localPort, udp::endpoint& serverEndpoint);

    void runUdpchat()
    {
        std::string input;
        while (running_)
        {
            std::shared_ptr<UdpChat> chat = currentChat_.load();
            if (chat)
            {
                std::getline(std::cin, input);
                std::cout << "> " << std::flush;
    
                chat->send(input);
                std::cout << "> " << std::flush;
            }
        }
    }


private:
    void startReceive();
    void startSend();
    void parseExternalIp();

    void parseResponse();

    std::array<uint8_t, 12> generateTransactionId();
    void sendWithRetry(int retryCount);

    void punching(uint32_t ip, uint16_t port);

    void startKcpTimer();

    void startWaitingSignals();
    
    boost::asio::io_context* ioctxPtr_;
    udp::socket socket_;
    boost::asio::signal_set signals_;
    udp::endpoint serverEndpoint_; //stun服务器

    boost::asio::steady_timer timer_;
    bool isResolved_;
    HolePunchClient holePunchClient_;
    // UdpChat udpChat_;
    

    udp::endpoint senderEndpoint_; //实际收到包的来源
    // std::array<char, 1024> recvBuffer_;
    boost::asio::streambuf sendStreambuf_;
    boost::asio::streambuf recvStreambuf_;

    std::array<uint8_t, 12> currentTransId_;  

    std::atomic<std::shared_ptr<UdpChat>> currentChat_;

    IUINT32 conv_;
    ikcpcb* kcp_;
   
    bool running_;
};