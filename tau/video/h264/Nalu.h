#pragma once

#include <cstdint>

namespace tau::h264 {

enum NaluType : uint8_t {
    kNone   = 0,
    kNonIdr = 1,
    kIdr    = 5,
    kSei    = 6,
    kSps    = 7,
    kPps    = 8,
    kAud    = 9,
    kStapA  = 24,
    kStapB  = 25,
    kMtap16 = 26,
    kMtap24 = 27,
    kFuA    = 28,
    kFuB    = 29
};

#pragma pack(push,1)

struct NaluHeader {
    uint8_t type      : 5;
    uint8_t nri       : 2;
    uint8_t forbidden : 1;
};

struct FuAHeader {
    uint8_t type     : 5;
    uint8_t reserved : 1;
    uint8_t end      : 1;
    uint8_t start    : 1;
};

#pragma pack(pop)

using FuAIndicator = NaluHeader;

inline uint8_t CreateNalUnitHeader(uint8_t type, uint8_t nri) {
    return ((nri & 0b00000011) << 5)
         | (type & 0b00011111);
}

inline uint8_t CreateFuAIndicator(uint8_t nalu_header) {
    return (nalu_header & 0b01100000) | NaluType::kFuA;
}

inline uint8_t CreateFuAHeader(bool start, bool end, uint8_t nalu_header) {
    return (start       ? 0b10000000 : 0)
         | (end         ? 0b01000000 : 0)
         | (nalu_header & 0b00011111);
}

}
