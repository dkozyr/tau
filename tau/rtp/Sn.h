#pragma once

#include <cstdint>

namespace rtp {

inline constexpr uint16_t kSnNegativeThreshold = 0x8000;

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

// sn in [left, right]
inline bool InRange(uint16_t sn, uint16_t left, uint16_t right) {
    if(SnLesser(sn, left) || SnLesser(right, sn)) {
        return false;
    }
    return true;
}

}
