#pragma once
#include <cstdint>

constexpr uint32_t STUN_MAGIC_COOKIE = 0x2112A442;

constexpr uint16_t STUN_BINDING_REQUEST  = 0x0001;
constexpr uint16_t STUN_BINDING_RESPONSE = 0x0101;


constexpr uint16_t ATTR_MAPPED_ADDRESS = 0x0001;
constexpr uint16_t ATTR_XOR_MAPPED_ADDRESS = 0x0020;

constexpr uint8_t STUN_REQUEST = 0xAF;
constexpr uint8_t EXTERNAL_IP = 0xBF;
constexpr uint8_t PUNCHING = 0xCF;
constexpr uint8_t MESSAGE = 0xDF;
constexpr uint8_t STOP = 0xEF;


