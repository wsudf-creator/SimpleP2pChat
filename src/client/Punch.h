#pragma once
#include <iostream>
#include <string>
#include <array>
#include <chrono>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

class HolePunchClient
{
public:
    HolePunchClient() = default;
    HolePunchClient(boost::asio::io_context& io_context, std::uint16_t local_port);

    void setRemoteEndpoint(uint32_t ip, uint16_t port)
    {
        boost::asio::ip::address_v4 remoteIp(ip);
        remoteEndpoint_ = udp::endpoint(boost::asio::ip::address(remoteIp), port);
    }

    bool isConnected() const
    {
        return isConnected_;
    }

    void setConnection()
    {
        isConnected_ = true;
    }

    void setNotConnection()
    {
        isConnected_ = false;
    }

    void startReceive(udp::socket& socket);
    void startPunching(udp::socket& socket);
    void sendPacket(udp::socket& socket, const std::string& msg);
private:
    // udp::socket socket_;
    udp::endpoint remoteEndpoint_; //对方的公网 IP:Port
    udp::endpoint senderEndpoint_; //实际收到包的来源
    std::array<char, 1024> recvBuffer_;
    boost::asio::steady_timer timer_;
    bool isConnected_;
};