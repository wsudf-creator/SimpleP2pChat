#include "StunClient.h"


#include <iomanip>
#include <ostream>
#include <random>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

void disableUdpConnReset(boost::asio::ip::udp::socket& socket) 
{
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    // 禁用 WSAECONNRESET (10054) 和 WSAECONNREFUSED (10061) 的响应
    ::WSAIoctl(socket.native_handle(), SIO_UDP_CONNRESET,
               &bNewBehavior, sizeof(bNewBehavior),
               NULL, 0, &dwBytesReturned, NULL, NULL);
}

#endif


StunClient::StunClient(boost::asio::io_context& io_context, uint16_t localPort, const std::string& remoteIp, uint16_t remotePort)
    : ioctxPtr_(&io_context), 
      socket_(io_context, udp::endpoint(udp::v4(), localPort)),
      signals_(io_context),
      serverEndpoint_(boost::asio::ip::make_address(remoteIp), remotePort),
      timer_(io_context),
      isResolved_(false),
      holePunchClient_(io_context, localPort)
{
#ifdef _WIN32
    // disableUdpConnReset(socket_);
#endif

    running_ = true;
    std::cout << "[Stun客户端]绑定至本地端口:" << localPort << std::endl;
    std::cout << "[Stun服务端]目标IP: " << remoteIp << ":" << remotePort << std::endl;

    kcp_ = nullptr;
    currentChat_.store(std::make_shared<UdpChat>(ioctxPtr_, &socket_));

    signals_.add(SIGINT);
    signals_.add(SIGTERM);

    startSend();
    startReceive();
    startWaitingSignals();
}

StunClient::StunClient(boost::asio::io_context& io_context, uint16_t localPort, udp::endpoint& serverEndpoint)
    : ioctxPtr_(&io_context), 
      socket_(io_context, udp::endpoint(udp::v4(), localPort)),
      signals_(io_context),
      serverEndpoint_(serverEndpoint),
      timer_(io_context),
      isResolved_(false),
      holePunchClient_(io_context, localPort)
{

#ifdef _WIN32
    // disableUdpConnReset(socket_);
#endif
    running_ = true;
    std::cout << "[Stun客户端]绑定至本地端口:" << localPort << std::endl;
    std::cout << "[Stun服务端]准备访问: " << serverEndpoint_.address().to_string() << std::endl;
    
    kcp_ = nullptr;
    currentChat_.store(std::make_shared<UdpChat>(ioctxPtr_, &socket_));

    signals_.add(SIGINT);
    signals_.add(SIGTERM);

    startSend();
    startReceive();
    startWaitingSignals();
}

void StunClient::startSend()
{
    //构建请求
    sendStreambuf_.consume(sendStreambuf_.size());
    std::ostream os(&sendStreambuf_);
    os.put(STUN_REQUEST);
    
    write_be16(os, STUN_BINDING_REQUEST);
    write_be16(os, 0);
    write_be32(os, STUN_MAGIC_COOKIE);
    
    currentTransId_ = generateTransactionId();
    std::cout << "Transaction ID:";
    for (auto c : currentTransId_)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    std::cout << std::endl;

    os.write(reinterpret_cast<const char*>(currentTransId_.data()), 12);

    socket_.async_send_to(sendStreambuf_.data(), serverEndpoint_, 
    [this](boost::system::error_code ec, std::size_t bytes){
       if (ec)
       {
        std::cerr << "[发送失败]" << ec.message() << std::endl;
       } 
    });
}

void StunClient::startReceive()
{
    recvStreambuf_.consume(recvStreambuf_.size());
    socket_.async_receive_from(recvStreambuf_.prepare(1024), senderEndpoint_, 
    [this](boost::system::error_code ec, std::size_t bytesRecvd){  
        recvStreambuf_.commit(bytesRecvd);
        std::istream is(&recvStreambuf_);
        uint8_t type;
        is.read((char*)&type, 1);

        if (type != 0xcf && type != 0xdf)
        {
            std::cout << "[类型]:" << std::hex << static_cast<int>(type) << std::endl;
        }

        if (!ec && type == STUN_REQUEST && bytesRecvd >= 20)
        {   
            parseResponse(); //得到本机的公网ip
        }
        else if (!ec && type == EXTERNAL_IP && bytesRecvd >= 6)
        {
            parseExternalIp(); //得到对方的公网ip并启动udp打洞
        }
        // else if (!ec && type == PUNCHING)
        // {
        //     std::istream is(&recvStreambuf_);
        //     std::string msg((std::istreambuf_iterator<char>(is)), 
        //                     std::istreambuf_iterator<char>());
        //     // std::cout << "\n[收到punch数据] 来自 " << senderEndpoint_ << " -> 内容:" << msg << std::endl;
        //     if (!holePunchClient_.isConnected())
        //     {
        //         holePunchClient_.setConnection();
        //         std::cout << ">>> [成功] P2P 直连通道已打通 <<<\n"; 
        //         // holePunchClient_.sendPacket(socket_, "P2P_ACK");
        //     }
    
        //     // currentChat_.load()->setTargetEndpoint(senderEndpoint_);
        // }
        else if (!ec && type == MESSAGE)  //kcp数据
        {
            if (currentChat_.load()->peerEndpoint() != senderEndpoint_)
            {
                std::cout << "[debug]:对端endpoint发生变更" << std::endl;
                currentChat_.load()->setTargetEndpoint(senderEndpoint_);
            }

            std::istream is(&recvStreambuf_);

            std::vector<char> rawData(
                (std::istreambuf_iterator<char>(is)), 
                std::istreambuf_iterator<char>()
            );

            int kcpRet = ikcp_input(kcp_, rawData.data(), rawData.size());
            if (kcpRet< 0) 
            {
                std::cerr << "[KCP Error] ikcp_input 解析失败，错误码: " << kcpRet << std::endl;
            }
            else 
            {
                char message[4096];
                int recvBytes = ikcp_recv(kcp_, message, sizeof(message));

                if (recvBytes > 0)
                {   
                    if (message[0] == UdpChat::MessageType::Messagiing)
                    {
                        std::string result(message + 1, recvBytes - 1);
                        std::cout << "\n[" << senderEndpoint_ << "]:" << result << std::endl;
       
                    }
                    else if (message[0] == UdpChat::MessageType::Heartbeating)
                    {
                        if (!holePunchClient_.isConnected())
                        {
                            holePunchClient_.setConnection();
                            std::cout << ">>> [成功] P2P 直连通道已打通 <<<\n"; 
                        }
                        std::cout << "[debug]:heartbeating from:" << senderEndpoint_ << std::endl;
                    }
                }
            }
        }
        else if (!ec && type == STOP)
        {
            std::cout << "[" << senderEndpoint_ << "]:" << "断开连接" << std::endl;
            holePunchClient_.setNotConnection();
            
            auto newChat = std::make_shared<UdpChat>(ioctxPtr_, &socket_);
            currentChat_.store(newChat);
            kcp_ = nullptr;
        }
        else if (!ec)
        {
           std::cout << "Unknown action" << std::endl;
        }
        else
        {
            if (ec.value() == 10061)
            {
                std::cerr << "[error]: 10061" << std::endl;
                std::cerr << "[senderEndpoint]:" << senderEndpoint_ << std::endl;
                
                if (currentChat_.load()->peerEndpoint() != senderEndpoint_)
                {
                    std::cout << "[debug]:对端endpoint发生变更" << std::endl;
                    currentChat_.load()->setTargetEndpoint(senderEndpoint_);
                }
            }
            else
            {
                std::cerr << "[startReceive:接收失败]:" << ec.what() << std::endl;
                std::cerr << "[message]:" << ec.message() << std::endl;
                std::cerr << "[value]:" << ec.value() << std::endl;
                std::cerr << "[senderEndpoint]:" << senderEndpoint_ << std::endl;
                return ;
            }
        }
        startReceive();
    });
}

void StunClient::startWaitingSignals()
{
    signals_.async_wait([this](const boost::system::error_code& ec, int signal){
        if (!ec)
        {
            running_ = false;
            std::cout << "[client]正在终止程序..." << std::endl;

            sendStreambuf_.consume(sendStreambuf_.size());
            std::ostream os(&sendStreambuf_);
            os.put(STOP);

            for (int i = 0; i < 3; i++)
            {
                socket_.send_to(sendStreambuf_.data(), serverEndpoint_);
            }

            if (holePunchClient_.isConnected())
            {
                for (int i = 0; i < 3; i++)
                {
                    socket_.send_to(sendStreambuf_.data(), currentChat_.load()->peerEndpoint());
                }
            }
            

            socket_.close();
            ioctxPtr_->stop();
        }
    });
}

void StunClient::parseResponse()
{
    std::istream is(&recvStreambuf_);

    uint16_t msgType = read_be16(is);
    uint16_t msgLength = read_be16(is);
    uint32_t magic = read_be32(is);

    // std::cout << "Message Type:" << std::hex << msgType << std::endl;
    // std::cout << "Message Length:" << std::hex << msgLength << std::endl;
    // std::cout << "Magic Cookie:" << std::hex << magic << std::endl;

    //跳过TransactionID
    is.ignore(12);

    if (msgType != STUN_BINDING_RESPONSE)
    {
        std::cerr << "非Binding Response响应" << std::endl;
        return ;
    }

    std::size_t bytesRead = 20; //已读Header 20字节
    std::size_t totalPayload = 20 + msgLength;

    //遍历TLV属性
    while (bytesRead + 4 <= totalPayload && is.good())
    {
        uint16_t attrType = read_be16(is);
        uint16_t attrLength = read_be16(is);
        bytesRead += 4;

        // std::cout << "Attribute Type:" << std::hex << attrType << std::endl;
        // std::cout << "Attribute Length:" << std::hex << attrLength << std::endl;

        if (bytesRead + attrLength > totalPayload)
            break;
        
        //处理XOR-MAPPED-ADDRESS
        if (attrType == ATTR_XOR_MAPPED_ADDRESS && attrLength >= 8)
        {
            is.ignore(1); //Reserved
            uint8_t family = is.get();

            if (family == 0x01) //IPv4
            {
                uint16_t xorPort = read_be16(is);
                uint32_t xorIp = read_be32(is);

                //XOR解密
                uint16_t realPort = xorPort ^ static_cast<uint16_t>(STUN_MAGIC_COOKIE >> 16);
                uint32_t realIp = xorIp ^ STUN_MAGIC_COOKIE;

                boost::asio::ip::address_v4 ipAddr(realIp);

                std::cout << "\n======================================" << std::endl;
                std::cout << "[成功获取公网地址 (XOR-MAPPED-ADDRESS)]" << std::endl;
                std::cout << "  - IP  : " << ipAddr.to_string() << std::endl;
                std::cout << "  - Port: " << realPort << std::endl;
                std::cout << "======================================\n" << std::endl;
            }
            else
            {
                is.ignore(attrLength - 2);
            }
        }
        else if (attrType == ATTR_MAPPED_ADDRESS && attrLength >= 8)
        {
            is.ignore(1); //Reserved
            uint8_t family = is.get();

            if (family == 0x01)
            {
                uint16_t real_port = read_be16(is);
                uint32_t real_ip   = read_be32(is);
                boost::asio::ip::address_v4 ip_addr(real_ip);

                std::cout << "[成功获取本机公网地址] " << ip_addr.to_string() << ":" << std::dec <<real_port << std::endl;
                return;
            }
            else
            {
                is.ignore(attrLength - 2);
            }
        }
        else
        {
            is.ignore(attrLength);
        }

        std::size_t pad = (4 - (attrLength % 4)) % 4;
        if (pad > 0)
        {
            is.ignore(pad);
        }

        bytesRead += attrLength + pad;
    }
}

std::array<uint8_t, 12> StunClient::generateTransactionId()
{
    std::array<uint8_t, 12> transId;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 255);

    for (size_t i = 0; i < 12; ++i)
    {
        transId[i] = static_cast<uint8_t>(dis(gen));
    }

    return transId;
}

void StunClient::parseExternalIp() //收到对方的endpoint
{
    std::istream is(&recvStreambuf_);

    uint32_t externalIp32 = read_be32(is);
    uint16_t externalPort32 = read_be16(is);
    uint32_t conv = read_be32(is);

    if (externalIp32 && externalPort32)
    {
        isResolved_ = true;
        boost::asio::ip::address_v4 externalIp(externalIp32);
        std::cout << "[穿透目标IP:Port]:" << externalIp.to_string() << ":" << std::dec << static_cast<int>(externalPort32) << std::endl;

        currentChat_.load()->setConv(conv);
        currentChat_.load()->createKcpConversation();
        kcp_ = currentChat_.load()->kcp();
        std::cout << "[kcp conv]:" << std::hex << conv << std::endl;

        //开始punch
        punching(externalIp32, externalPort32);
    }
    else
    {
        std::cout << "获取IP:Port失败, 请重试" << std::endl;
    }
}

void StunClient::punching(uint32_t ip, uint16_t port)
{
    // holePunchClient_.setRemoteEndpoint(ip, port);
    // holePunchClient_.startPunching(socket_);
    boost::asio::ip::address_v4 remoteIp(ip);
    udp::endpoint target(boost::asio::ip::address(remoteIp), port);
    currentChat_.load()->setTargetEndpoint(target);
    currentChat_.load()->startKcpTimer();
    currentChat_.load()->startHeartbeatLoop();
}
