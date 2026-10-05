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
    enum MessageType
    {
        Messagiing = 0x11, 
        Heartbeating
    };

    UdpChat() = default;
    UdpChat(boost::asio::io_context* ioctxPtr, udp::socket* socket);
    void send(const std::string& message, MessageType type);
    void send(void* data, int size);

    void createKcpConversation();
    void sendHeartbeat();
    void startKcpTimer();
    void startHeartbeatLoop();

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

    static uint32_t getCurrentTimestampMs() 
    {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        // 转换为毫秒并强转为 32 位无符号整数
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
        );
    }

private:
    boost::asio::io_context* ioctx_;
    udp::socket* socket_;
    udp::endpoint peer_endpoint_;   // 对方地址
    udp::endpoint sender_endpoint_; // 实际接收到的消息来源地址
    IUINT32 conv_;
    ikcpcb* kcp_;
    std::array<char, 1024> recv_buffer_;
    boost::asio::steady_timer heartbeat_timer_;
    boost::asio::steady_timer kcp_timer_;
    std::chrono::steady_clock::time_point last_active_time_;
};
