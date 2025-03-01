#pragma once

#include <cstdint>
#include <limits>

namespace rtp {

inline constexpr uint16_t kSnNegativeThreshold = 0x8000;

struct SnRange {
    uint16_t left;
    uint16_t right;
};

inline uint16_t SnForward(uint16_t sn, uint16_t delta) {
    return sn + delta;
}

inline uint16_t SnBackward(uint16_t sn, uint16_t delta) {
    return sn - delta;
}

inline uint16_t SnDelta(uint16_t a, uint16_t b) {
    return a - b;
}

inline uint16_t SnLesser(uint16_t a, uint16_t b) {
    return (SnDelta(a, b) >= kSnNegativeThreshold);
}

inline uint16_t SnGreater(uint16_t a, uint16_t b) {
    return SnLesser(b, a);
}

inline bool InRange(const SnRange& range, uint16_t sn) {
    if(SnLesser(sn, range.left)) {
        return false;
    }
    if(SnLesser(range.right, sn)) {
        return false;
    }
    return true;
}

}
