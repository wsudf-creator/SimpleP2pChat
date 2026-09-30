#pragma once
#include <cstdint>
#include<istream>

#include "../utility/utility.h"

class StunRequest
{
public:
    StunRequest(std::istream& is)
    {
        msgType_ = read_be16(is);
        msgLength_ = read_be16(is);
        magiCookie_ = read_be32(is);

        is.read(reinterpret_cast<char*>(transID_.data()), 12);
    }

    uint16_t messageType() const
    {
        return msgType_;
    }

    uint16_t messageLength() const
    {
        return msgLength_;
    }

    uint32_t magiCookie() const 
    {
        return magiCookie_;
    }

    const std::array<uint8_t, 12>& transID() const
    {
        return transID_;
    }

    void setXorEndpoint(uint32_t ip, uint16_t port)
    {
        xorPort_ = port;
        xorIp_ = ip;
    }

    uint32_t xorIp() const
    {
        return xorIp_;
    }

    uint16_t xorPort() const
    {
        return xorPort_;
    }

private:
    uint16_t msgType_;
    uint16_t msgLength_;
    uint32_t magiCookie_;
    std::array<uint8_t, 12> transID_;
    uint16_t xorPort_;
    uint32_t xorIp_;
};