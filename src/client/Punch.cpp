#include "Punch.h"
#include <random>

HolePunchClient::HolePunchClient(boost::asio::io_context& io_context, std::uint16_t local_port)
    : timer_(io_context), 
      isConnected_(false)
{
    // std::cout << "[UDP punching] 绑定本地端口: " << local_port + 1 << std::endl;
}




void HolePunchClient::startPunching(udp::socket& socket)
{    
    sendPacket(socket, "PUNCH_HOLE");
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(3000, 5000);

    int duration_ms = dist(gen);

    timer_.expires_after(std::chrono::milliseconds(duration_ms));
    timer_.async_wait([this, &socket](boost::system::error_code ec){
        if (!ec)
        {
            startPunching(socket);
        }
    });
}

void HolePunchClient::sendPacket(udp::socket& socket, const std::string& msg)
{
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os.put(0xcf); //PUNCHING
    os.write(msg.c_str(), msg.size());

#ifdef _WIN32
    // std::cout << "[SEND] Sending heartbeat to " << remoteEndpoint_ << " ... " << std::endl;
#endif
    
    socket.async_send_to(buf.data(), remoteEndpoint_, [](boost::system::error_code ec, std::size_t bytes){
#ifdef _WIN32  
        // std::cout << "[SEND CALLBACK] ec: " << ec.message() << ", bytes: " << bytes << std::endl;  
#endif
    });
}