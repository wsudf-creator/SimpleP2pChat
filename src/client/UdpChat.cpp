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

UdpChat::UdpChat(udp::socket* socket)
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
void UdpChat::send(const std::string& message) 
{
    boost::asio::post(socket_->get_executor(), [this, message](){
        if (peer_endpoint_.size() == 0)
        {
            std::cout << "[Invalid peer address]" << std::endl;
            return ;
        }

        auto send_data = std::make_shared<std::string>();
        // send_data->push_back(static_cast<char>(0xdf)); // 插入自定义字节
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
