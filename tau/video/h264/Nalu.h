#pragma once

#include <cstdint>

namespace h264 {

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

struct NaluHeader {
    uint8_t type      : 5;
    uint8_t nri       : 2;
    uint8_t forbidden : 1;
};

inline uint8_t BuildNalUnitHeader(uint8_t type, uint8_t nri) {
    return ((nri & 0b00000011) << 5)
         | (type & 0b00011111);
}

}
