#pragma once

#include <cstdint>
#include <cstddef>

namespace tau::rtp {

inline constexpr size_t kAv1AggregationHeaderSize = sizeof(uint8_t);

inline constexpr uint8_t kAv1ContinuationMask  = 0b10000000;
inline constexpr uint8_t kAv1ContinuesMask     = 0b01000000;
inline constexpr uint8_t kAv1ElementCountMask  = 0b00110000;
inline constexpr uint8_t kAv1ElementCountShift = 4;
inline constexpr uint8_t kAv1NewSequenceMask   = 0b00001000;

}
