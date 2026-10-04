#pragma once

#include "ikcp.h"
#include <iostream>
#include <string>
#include <array>
#include <thread>
#include <boost/asio.hpp>


using boost::asio::ip::udp;

class UdpChat
{
public:
    UdpChat() = default;
    UdpChat(udp::socket* socket);
    void send(const std::string& message);
    void send(void* data, int size);
    void run();

    void setTargetEndpoint(udp::endpoint peer_endpoint)
    {
        peer_endpoint_ = peer_endpoint;
    }

    udp::socket* socket()
    {
        return socket_;
    }

    const udp::endpoint& peerEndpoint() const
    {
        return peer_endpoint_;
    }

    IUINT32 conv()
    {
        return conv_;
    }

    ikcpcb* kcp() const
    {
        if (!kcp_)
        {
            std::cerr << "[UdpChat]:Invalid kcp instance" << std::endl;
            return nullptr;
        }
        return kcp_;
    }

    void setConv(IUINT32 conv)
    {
        conv_ = conv;
    }

    void createKcpConversation();


private:    
    
    udp::socket* socket_;
    udp::endpoint peer_endpoint_;   // 对方地址
    udp::endpoint sender_endpoint_; // 实际接收到的消息来源地址
    IUINT32 conv_;
    ikcpcb* kcp_;
    std::array<char, 1024> recv_buffer_;
};
