#pragma once

#include <cstdint>
#include <cstddef>

namespace tau::rtp {

#pragma pack(push,1)

struct FuHeader {
    uint8_t type     : 6;
    uint8_t end      : 1;
    uint8_t start    : 1;
};

#pragma pack(pop)

inline uint8_t CreateFuHeader(bool start, bool end, uint8_t nalu_type) {
    return (start     ? 0b10000000 : 0)
         | (end       ? 0b01000000 : 0)
         | (nalu_type & 0b00111111);
}

}
