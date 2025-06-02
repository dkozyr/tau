#pragma once

#include <cstdint>
#include <cstddef>

namespace tau::h265 {

enum NaluType : uint8_t {
    kFu      = 48,
    kAp      = 49,
};

inline constexpr size_t kNaluSize = sizeof(uint16_t);

inline constexpr size_t kNaluForbiddenMask = 0b10000000;
inline constexpr size_t kNaluTypeMask      = 0b01111110;
inline constexpr size_t kNaluLayerMask0    = 0b00000001;
inline constexpr size_t kNaluLayerMask1    = 0b11111000;
inline constexpr size_t kNaluTidMask       = 0b00000111;

//TODO: unit tests
inline NaluType GetNaluTypeUnsafe(const uint8_t* data) {
    return static_cast<NaluType>((data[0] & kNaluTypeMask) >> 1);
}

inline uint8_t GetNaluLayerUnsafe(const uint8_t* data) {
    return ((data[0] & kNaluLayerMask0) << 5) | ((data[1] & kNaluLayerMask1) >> 3);
}

inline uint8_t GetNaluTidUnsafe(const uint8_t* data) {
    return (data[1] & kNaluTidMask);
}

}
