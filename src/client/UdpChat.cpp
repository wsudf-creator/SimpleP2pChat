#include "UdpChat.h"


int udpOutputCallback(const char* buf, int len, ikcpcb* kcp, void* user)
{
    auto* client = static_cast<UdpChat*>(user);

    std::vector<char> sendBuf(len + 1);
    sendBuf[0] = static_cast<char>(0xdf);
    std::memcpy(sendBuf.data() + 1, buf, len);

    client->socket()->async_send_to(
        boost::asio::buffer(sendBuf), client->peerEndpoint(),
        [](const boost::system::error_code& ec, std::size_t){
            if (ec) 
            {
                std::cerr << "\n[udpOutputCallback]发送失败: " << ec.message() << "\n> " << std::flush;
            }
        }
    );

    return 0;
}

UdpChat::UdpChat(boost::asio::io_context* ioctxPtr, udp::socket* socket)
    : ioctx_(ioctxPtr),
      heartbeat_timer_(*ioctxPtr),
      kcp_timer_(*ioctxPtr),
      last_active_time_(std::chrono::steady_clock::now())
{
    socket_ = socket;
}

void UdpChat::createKcpConversation()
{
    if (!conv_)
    {
        std::cerr << "[UdpChat::createKcpConversation]:Invalid conv" << std::endl;
    }

    kcp_ = ikcp_create(conv_, static_cast<void*>(this));
    kcp_->output = udpOutputCallback;
    std::cout << "=== UDP 聊天已启动 ===" << std::endl;
    std::cout << "请输入消息后按回车发送：\n" << std::endl;
}

    // 发送消息到目标节点
void UdpChat::send(const std::string& message, MessageType type) 
{
    boost::asio::post(socket_->get_executor(), [this, message, type](){
        if (peer_endpoint_.size() == 0)
        {
            std::cout << "[Invalid peer address]" << std::endl;
            return ;
        }

        auto send_data = std::make_shared<std::string>();
        send_data->push_back(static_cast<char>(type)); // 插入自定义字节
        send_data->append(message);                          // 拼接文本

        ikcp_send(kcp_, send_data->c_str(), send_data->size());

    });
}

void UdpChat::send(void* data, int size)
{
    boost::asio::post(socket_->get_executor(), [this, data, size](){
        if (peer_endpoint_.size() == 0)
        {
            std::cout << "[Invalid peer address]" << std::endl;
            return ;
        }

        ikcp_send(kcp_, (const char*)data, size);
    });
}


void UdpChat::sendHeartbeat() 
{
    uint8_t cmd = MessageType::Heartbeating; // 只有一个字节的命令字 0x01
    
    // 走 ikcp_send 发送，KCP 会自动为其加上 24 字节的 KCP 头部
    int ret = ikcp_send(kcp_, reinterpret_cast<const char*>(&cmd), sizeof(cmd));
    if (ret < 0) {
        std::cerr << "[KCP] 心跳包入队失败, code: " << ret << std::endl;
    }
}

void UdpChat::startKcpTimer()
{
    kcp_timer_.expires_after(std::chrono::milliseconds(10));
    kcp_timer_.async_wait([this](boost::system::error_code ec) {
        if (!ec) 
        {
            // 获取当前毫秒级时间戳驱动 KCP 状态机
            IUINT32 current_ms = getCurrentTimestampMs();

            ikcp_update(kcp_, current_ms);

            // 递归循环定时器
            startKcpTimer();
        }
    });
}

void UdpChat::startHeartbeatLoop()
{
    heartbeat_timer_.expires_after(std::chrono::seconds(1));
    heartbeat_timer_.async_wait([this](boost::system::error_code ec){
        if (!ec)
        {
            sendHeartbeat();
            startHeartbeatLoop();
        }
    });
}