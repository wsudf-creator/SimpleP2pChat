#pragma once
#include <ostream>

#include <iomanip>
#include <algorithm>

class StunResponse
{
public:
    StunResponse(std::ostream& os)
    {
        os_ = &os;
    }

    void setTransactionID(const std::array<uint8_t, 12>& trans_id)
    {
        transactionID_ =  trans_id;
    }

    void setType()
    {
        os_->put(STUN_REQUEST);
    }

private:
    void setMessageType(uint16_t type)
    {
        messageType_ = type;
    }

    void setMessageLength(int length)
    {
        messageLength_ = length;
    }

    void setMagiCookie(uint32_t cookie)
    {
        magiCookie_ = cookie;
    }

    void setAttributeType(uint16_t type)
    {
        attributeType_ = type;
    }

    void setAttriValLen(int len)
    {
        attributeValLen_ = len;
    }

    void setReserved(unsigned char val)
    {
        reserved_ = val;
    }

    void setFamily(unsigned char val)
    {
        family_ = val;
    }

public:
    void setPort(uint16_t port)
    {
        rawPort_ = port;
        xorPort_ = rawPort_ ^ static_cast<uint16_t>(STUN_MAGIC_COOKIE >> 16);
    }

    void setIp(uint32_t ip)
    {
        rawIp_ = ip;
        xorIp_ = rawIp_ ^ STUN_MAGIC_COOKIE;
    }
    void setResponseHeaders(uint16_t type, int len)
    {
        //STUN Header (20 字节)
        setMessageType(type); // Message Type
        setMessageLength(len);                  // Message Length (属性长度为 12B)
        setMagiCookie(STUN_MAGIC_COOKIE);    // Magic Cookie

        write_be16(*os_, messageType_);
        write_be16(*os_, messageLength_);
        write_be32(*os_, magiCookie_);
        os_->write(reinterpret_cast<const char*>(transactionID_.data()), 12);
    }

    void setAttrXORMappedAddress4(uint8_t reserved)
    {
        // XOR-MAPPED-ADDRESS Attribute (12 字节)
        setAttributeType(ATTR_XOR_MAPPED_ADDRESS); // Attribute Type (0x0020)
        setAttriValLen(8);                  // Attribute Value Length (8B)
        setReserved(reserved);                            // Reserved
        setFamily(0x01);                            // Family: IPv4

        write_be16(*os_, ATTR_XOR_MAPPED_ADDRESS);
        write_be16(*os_, attributeValLen_);
        os_->put(reserved_);
        os_->put(family_);
        write_be16(*os_, xorPort_);
        write_be32(*os_, xorIp_);
    }

    void setAttrMappedAddress4(uint8_t reserved)
    {
        setAttributeType(ATTR_MAPPED_ADDRESS); // Attribute Type (0x0020)
        setAttriValLen(8);                  // Attribute Value Length (8B)
        setReserved(reserved);                            // Reserved
        setFamily(0x01);  

        // 12 字节
        write_be16(*os_, ATTR_MAPPED_ADDRESS);
        write_be16(*os_, attributeValLen_);
        os_->put(reserved);
        os_->put(0x01);
        write_be16(*os_, rawPort_);
        write_be32(*os_, rawIp_);
    }

    void showResponses()
    {
        std::cout << std::hex << "Message Type:"<< messageType_ << std::endl;
        std::cout << std::dec << "Message Length:"<< messageLength_ << std::endl;
        std::cout << std::hex << "Magic Cookie:"<< magiCookie_ << std::endl;
        
        for (auto c : transactionID_)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
        std::cout << std::endl;

        std::cout << std::hex << "Attribute Type:"<< attributeType_ << std::endl;
        std::cout << std::hex << "Attribute Value Length:"<< attributeValLen_ << std::endl;
        std::cout << "Reserved:"<< std::hex << std::setw(2) << std::setfill('0')  << static_cast<int>(reserved_) << std::endl;
        std::cout << "Family:" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(family_) << std::endl;
        std::cout << std::hex << "XOR PORT:"<< xorPort_ << std::endl;
        std::cout << std::hex << "XOR IP:"<< xorIp_ << std::endl;

        std::cout << std::hex << "RAW PORT:"<< rawPort_ << std::endl;
        std::cout << std::hex << "RAW IP:"<< rawIp_ << std::endl;
    }


private: 
    std::ostream* os_;
    uint16_t messageType_;
    uint16_t messageLength_;
    uint32_t magiCookie_;
    std::array<uint8_t, 12> transactionID_;
    uint16_t attributeType_;
    uint16_t attributeValLen_;
    uint8_t reserved_;
    uint8_t family_;
    uint16_t xorPort_;
    uint32_t xorIp_;

    uint16_t rawPort_;
    uint32_t rawIp_;
};