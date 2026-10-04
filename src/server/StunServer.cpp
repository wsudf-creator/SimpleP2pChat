#include "StunServer.h"
#include <random>
using boost::asio::ip::udp;

void StunServer::setStunCallback(const StunCallback& callback)
{
    stunCallback_ = callback;
}

StunServer::StunServer(boost::asio::io_context& io_context, uint16_t port)
    : socket_(io_context, udp::endpoint(udp::v4(), port)) 
{
    got_first = false;

    std::cout << "[STUN Server] 监听端口: " << port << "..." << std::endl;
    start_receive();
}


void StunServer::start_receive() 
{
    // 重置/清理 streambuf 准备接收新数据
    recv_streambuf_.consume(recv_streambuf_.size());
    // 使用 prepare 分配写入空间，直接读取到 streambuf 中
    socket_.async_receive_from(
        recv_streambuf_.prepare(1024), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_recvd) {
            if (!ec) 
            {   
                recv_streambuf_.commit(bytes_recvd);

                std::istream is(&recv_streambuf_);
                uint8_t type;
                is.read((char*)&type, 1); //消耗类型
                // recv_streambuf_.consume(1);

                if (type == STOP && active_ends_.find(remote_endpoint_) != active_ends_.end())
                {
                    active_ends_.erase(remote_endpoint_);
                    remote_ends_.erase(remote_endpoint_);

                    if (got_first && remote_endpoint_ == first_endpoint_) //first_endpoint_断连
                    {
                        got_first = false;
                        first_endpoint_ = udp::endpoint();
     
                        if (active_ends_.size() > 0)
                        {
                            remote_ends_ = active_ends_;
                            // send_streambuf_.consume(send_streambuf_.size());
                            // std::ostream os(&send_streambuf_);
                            // os.put(MESSAGE);
                            
                            // std::string msg("server endpoint closed the connection\n");
                            // std::cout << "[StunServer]:" << msg << std::endl;
                            // os.write(msg.c_str(), msg.size());

                            // for (auto& end : active_ends_)
                            // {
                            //     socket_.async_send_to(send_streambuf_.data(), end, [](boost::system::error_code ec, std::size_t){
                            //         if (ec)
                            //         {
                            //             std::cerr << ec.what() << std::endl;
                            //         }
                            //     });
                            // }
                        }
                    }
                }
                else if (bytes_recvd >= 20)
                {
                    handle_request();
                    if (!got_first && remote_endpoint_ != first_endpoint_)
                    {
                        first_endpoint_ = remote_endpoint_;
                        got_first = true;
                        active_ends_.insert(remote_endpoint_);
                        if (remote_ends_.size() > 0)
                            exchange_end();
                    }
                    else if(got_first && remote_endpoint_ != first_endpoint_)
                    {
                        remote_ends_.insert(remote_endpoint_);
                        active_ends_.insert(remote_endpoint_);
                        if (remote_ends_.size() > 0)
                            exchange_end();
                    }
                }
            }
            
            start_receive();
        });
}

void StunServer::handle_request() 
{
    // 使用 std::istream 读取接收到的 streambuf
    std::istream is(&recv_streambuf_);
    StunRequest request(is);

    std::cout << "[收到请求] 来自: " << remote_endpoint_.address().to_string() 
                << ":" << remote_endpoint_.port() << std::endl;

    // --- 构造响应：使用 send_streambuf_ ---
    send_streambuf_.consume(send_streambuf_.size()); // 清空发送缓冲区
    std::ostream os(&send_streambuf_);
    StunResponse resp(os);

    // 计算 XOR-MAPPED-ADDRESS 属性数据
    uint16_t raw_port = remote_endpoint_.port();
    uint16_t xor_port = raw_port ^ static_cast<uint16_t>(STUN_MAGIC_COOKIE >> 16);

    uint32_t raw_ip = remote_endpoint_.address().to_v4().to_uint();
    uint32_t xor_ip = raw_ip ^ STUN_MAGIC_COOKIE;

    resp.setIp(raw_ip);
    resp.setPort(raw_port);
    resp.setTransactionID(request.transID());

    request.setXorEndpoint(xor_ip, xor_port);

    stunCallback_(request, resp);

    // 异步发送数据 (直接将 streambuf.data() 传入 buffer)
    socket_.async_send_to(
    send_streambuf_.data(), remote_endpoint_,
    [this](boost::system::error_code ec, std::size_t) {
        if (ec) {
            std::cerr << "[发送失败]: " << ec.message() << std::endl;
        }
    });
}

void StunServer::exchange_end()
{ 
    std::shared_ptr<std::array<uint8_t, 4>> convPtr = std::make_shared<std::array<uint8_t, 4>>(generateConv());

    for (auto it = remote_ends_.begin(); it != remote_ends_.end();)
    {
        auto end = *it;
        if (first_endpoint_ == end)
        {
            it = remote_ends_.erase(it);
            continue;
        }

        std::cout << "[交换IP:Port]:" << first_endpoint_.address().to_v4().to_string() << ":" << first_endpoint_.port() << "<->"
                  << end.address().to_v4().to_string() << ":" << end.port() << std::endl;
        send_streambuf_.consume(send_streambuf_.size());
        std::ostream fos(&send_streambuf_);

        uint32_t ip = end.address().to_v4().to_uint();
        uint16_t port = end.port();
        fos.put(EXTERNAL_IP);
        write_be32(fos, ip);
        write_be16(fos, port);
        fos.write(reinterpret_cast<const char*>(convPtr->data()), 4);

        socket_.async_send_to(
        send_streambuf_.data(), first_endpoint_,
        [this](boost::system::error_code ec, std::size_t) {
            if (ec)
            {
                std::cerr << "[交换失败]: " << ec.message() << std::endl;
            }
        }
        );

        send_streambuf_.consume(send_streambuf_.size());
        std::ostream oos(&send_streambuf_);

        ip = first_endpoint_.address().to_v4().to_uint();
        port = first_endpoint_.port();
        oos.put(EXTERNAL_IP);
        write_be32(oos, ip);
        write_be16(oos, port);
        oos.write(reinterpret_cast<const char*>(convPtr->data()), 4);

        socket_.async_send_to(
        send_streambuf_.data(), end,
        [this](boost::system::error_code ec, std::size_t) {
            if (ec)
            {
                std::cerr << "[交换失败]: " << ec.message() << std::endl;
            }
        }
        );

        it = remote_ends_.erase(it);
        // ++it;
    }
}

std::array<uint8_t, 4> StunServer::generateConv()
{
    std::array<uint8_t, 4> conv;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 255);

    for (size_t i = 0; i < 4; ++i)
    {
        conv[i] = static_cast<uint8_t>(dis(gen));
    }

    return conv;
}