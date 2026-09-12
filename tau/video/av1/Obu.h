#pragma once

#include <tau/memory/BufferView.h>
#include <optional>
#include <cstdint>
#include <optional>

namespace tau::av1 {

inline constexpr uint8_t kObuForbiddenMask         = 0b10000000;
inline constexpr uint8_t kObuTypeMask              = 0b01111000;
inline constexpr uint8_t kObuExtensionMask         = 0b00000100;
inline constexpr uint8_t kObuHasSizeMask           = 0b00000010;
inline constexpr uint8_t kObuReservedMask          = 0b00000001;
inline constexpr uint8_t kObuExtensionReservedMask = 0b00000111;

enum ObuType : uint8_t {
    kReserved0            = 0,
    kSequenceHeader       = 1,
    kTemporalDelimiter    = 2,
    kFrameHeader          = 3,
    kTileGroup            = 4,
    kMetadata             = 5,
    kFrame                = 6,
    kRedundantFrameHeader = 7,
    kTileList             = 8,
    kReserved9            = 9,
    kReserved10           = 10,
    kReserved11           = 11,
    kReserved12           = 12,
    kReserved13           = 13,
    kReserved14           = 14,
    kPadding              = 15
};

struct Obu {
    BufferViewConst header;
    BufferViewConst payload;
    size_t size = 0;

    uint8_t Type() const { return (header.ptr[0] & kObuTypeMask) >> 3; }
    bool Ignored() const { return (Type() == ObuType::kTemporalDelimiter) || (Type() == ObuType::kTileList); }
};

std::optional<Obu> ReadObu(BufferViewConst input, bool require_size = false);

}
