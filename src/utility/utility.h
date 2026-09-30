#pragma once
#include <cstdint>
#include <istream>
#include <chrono>

// 网络字节序转换工具方法
inline uint16_t read_u16(const uint8_t* ptr) {
    return (static_cast<uint16_t>(ptr[0]) << 8) | ptr[1];
}

inline void write_u16(uint8_t* ptr, uint16_t val) {
    ptr[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[1] = static_cast<uint8_t>(val & 0xFF);
}

inline void write_u32(uint8_t* ptr, uint32_t val) {
    ptr[0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    ptr[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    ptr[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[3] = static_cast<uint8_t>(val & 0xFF);
}



// 工具函数：从输入流中读取大端序（网络字节序）整数
inline uint16_t read_be16(std::istream& is) {
    uint8_t b[2];
    is.read(reinterpret_cast<char*>(b), 2);
    return (static_cast<uint16_t>(b[0]) << 8) | b[1];
}

inline uint32_t read_be32(std::istream& is) {
    uint8_t b[4];
    is.read(reinterpret_cast<char*>(b), 4);
    return (static_cast<uint32_t>(b[0]) << 24) |
           (static_cast<uint32_t>(b[1]) << 16) |
           (static_cast<uint32_t>(b[2]) << 8)  | b[3];
}

// 工具函数：向输出流中写入大端序（网络字节序）整数
inline void write_be16(std::ostream& os, uint16_t val) {
    uint8_t b[2] = { static_cast<uint8_t>((val >> 8) & 0xFF), static_cast<uint8_t>(val & 0xFF) };
    os.write(reinterpret_cast<char*>(b), 2);
}

inline void write_be32(std::ostream& os, uint32_t val) {
    uint8_t b[4] = {
        static_cast<uint8_t>((val >> 24) & 0xFF),
        static_cast<uint8_t>((val >> 16) & 0xFF),
        static_cast<uint8_t>((val >> 8) & 0xFF),
        static_cast<uint8_t>(val & 0xFF)
    };
    os.write(reinterpret_cast<char*>(b), 4);
}
