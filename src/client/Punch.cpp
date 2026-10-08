#include "Punch.h"

HolePunchClient::HolePunchClient(boost::asio::io_context& io_context, std::uint16_t local_port)
    : timer_(io_context), 
      isConnected_(false)
{
    // std::cout << "[UDP punching] 绑定本地端口: " << local_port + 1 << std::endl;
}




void HolePunchClient::startPunching(udp::socket& socket)
{    
    sendPacket(socket, "PUNCH_HOLE");
    
    //每隔1秒发送一次心跳打洞包
    int duration;
    duration = 1;

    timer_.expires_after(std::chrono::seconds(duration));
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

    socket.async_send_to(buf.data(), remoteEndpoint_, [](boost::system::error_code ec, std::size_t){
        if (ec)
        {
            std::cerr << "[发送失败]: " << ec.message() << std::endl;
        }
    });
}