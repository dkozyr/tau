#pragma once

#include <cstdint>

inline uint16_t Read16(const uint8_t* ptr) {
    return (static_cast<uint16_t>(ptr[0]) << 8) | ptr[1];
}

inline uint32_t Read32(const uint8_t* ptr) {
    return (static_cast<uint32_t>(Read16(ptr)) << 16) | Read16(ptr + sizeof(uint16_t));
}

inline uint64_t Read64(const uint8_t* ptr) {
    return (static_cast<uint64_t>(Read32(ptr)) << 32) | Read32(ptr + sizeof(uint32_t));
}

inline void Write16(uint8_t* ptr, uint16_t value) {
    ptr[0] = (value >> 8);
    ptr[1] = (value & 0xFF);
}

inline void Write32(uint8_t* ptr, uint32_t value) {
    ptr[0] = (value >> 24) & 0xFF;
    ptr[1] = (value >> 16) & 0xFF;
    ptr[2] = (value >> 8 ) & 0xFF;
    ptr[3] = (value      ) & 0xFF;
}

inline void Write64(uint8_t* ptr, uint64_t value) {
    ptr[0] = (value >> 56) & 0xFF;
    ptr[1] = (value >> 48) & 0xFF;
    ptr[2] = (value >> 40) & 0xFF;
    ptr[3] = (value >> 32) & 0xFF;
    ptr[4] = (value >> 24) & 0xFF;
    ptr[5] = (value >> 16) & 0xFF;
    ptr[6] = (value >> 8 ) & 0xFF;
    ptr[7] = (value      ) & 0xFF;
}
