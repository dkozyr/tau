#pragma once

#include <cstdint>
#include <cstddef>

namespace tau::h265 {

enum NaluType : uint8_t {
    kTrailN       = 0,
    kTrailR       = 1,
    kTsaN         = 2,
    kTsaR         = 3,
    kStsaN        = 4,
    kStsaR        = 5,
    kRadlN        = 6,
    kRadlR        = 7,
    kBlaWLp       = 16,
    kBlaWRadl     = 17,
    kBlaNLp       = 18,
    kIdrWRadl     = 19,
    kIdrNLp       = 20,
    kCra          = 21,
    kRsvIrapVcl23 = 23,
    kVps          = 32,
    kSps          = 33,
    kPps          = 34,
    kAud          = 35,
    kPrefixSei    = 39,
    kSuffixSei    = 40,
    kAp           = 48, // section 4.4.2 in RFC 7798.
    kFu           = 49, // section 4.4.3 in RFC 7798.
    kPaci         = 50, // section 4.4.4 in RFC 7798.
};

inline constexpr size_t kNaluHeaderSize = sizeof(uint16_t);

inline constexpr size_t kNaluForbiddenMask = 0b10000000;
inline constexpr size_t kNaluTypeMask      = 0b01111110;
inline constexpr size_t kNaluLayerMask0    = 0b00000001;
inline constexpr size_t kNaluLayerMask1    = 0b11111000;
inline constexpr size_t kNaluTidMask       = 0b00000111;

inline void SetNaluHeaderUnsafe(uint8_t* data, NaluType type, uint8_t layer_id, uint8_t tid) {
    data[0] = ((type << 1) & kNaluTypeMask) | ((layer_id >> 5) & kNaluLayerMask0);
    data[1] = ((layer_id << 3) & kNaluLayerMask1) | (tid & kNaluTidMask);
}

inline void SetNaluHeaderTypeUnsafe(uint8_t* data, NaluType type) {
    data[0] = (data[0] & (~kNaluTypeMask)) | ((type << 1) & kNaluTypeMask);
}

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
