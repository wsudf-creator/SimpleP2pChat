#pragma once

#include <iostream>
#include <array>
#include <set>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <boost/asio.hpp>
#include <functional>
#include "../utility/ConstDef.h"

#include "StunRequest.h"
#include "StunResponse.h"

using boost::asio::ip::udp;

class StunServer
{
public:
    typedef std::function<void(StunRequest&, StunResponse&)> StunCallback;

    StunServer(boost::asio::io_context& io_context, uint16_t port);

    void setStunCallback(const StunCallback& callback);

private:
    void start_receive();
    void handle_request();
    void exchange_end();

    std::array<uint8_t, 4> generateConv();

    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    udp::endpoint first_endpoint_;
    boost::asio::streambuf recv_streambuf_;
    boost::asio::streambuf send_streambuf_;
    StunCallback stunCallback_;
    std::set<udp::endpoint> remote_ends_;
    std::set<udp::endpoint> active_ends_;
    bool got_first;
};
