#pragma once

#include "tau/memory/BufferView.h"

namespace tau::rtcp {

inline constexpr size_t kHeaderSize = sizeof(uint32_t);
inline constexpr uint8_t kVersion2          = 0b10000000;
inline constexpr uint8_t kHeaderVersionMask = 0b11000000;
inline constexpr uint8_t kHeaderPaddingMask = 0b00100000;
inline constexpr uint8_t kHeaderRcMask      = 0b00011111;

enum Type : uint8_t {
    kSr    = 200,  // Sender Report,           RFC-3550
    kRr    = 201,  // Receiver Report,         RFC-3550
    kSdes  = 202,  // Source Description,      RFC-3550
    kBye   = 203,  // Goodbye,                 RFC-3550
    kApp   = 204,  // Application-Defined,     RFC-3550
    kRtpfb = 205,  // Generic RTP Feedback,    RFC-4585
    kPsfb  = 206,  // Payload-specific,        RFC-4585
    kXr    = 207,  // RTCP Extension,          RFC-3611
};

enum RtpfbType : uint8_t {
    kNack  = 1,    // Generic Nack,            RFC-4585
};

enum PsfbType : uint8_t {
    kPli   = 1,    // Picture Loss Indication, RFC-4585
    kFir   = 4,    // Full Intra Request,      RFC-5104
};

inline bool ValidateVersion(uint8_t header_byte) {
    return ((header_byte & kHeaderVersionMask) == kVersion2);
}

inline bool HasPadding(uint8_t header_byte) {
    return ((header_byte & kHeaderPaddingMask) == kHeaderPaddingMask);
}

inline uint8_t GetRc(uint8_t header_byte) {
    return (header_byte & kHeaderRcMask);
}

inline bool IsRtcp(const BufferViewConst& view) {
    if((view.size >= kHeaderSize) && ValidateVersion(view.ptr[0])) {
        const auto pt = view.ptr[1];
        return ((pt >= Type::kSr) && (pt <= Type::kXr));
    }
    return false;
}

inline uint8_t BuildHeader(uint8_t rc, bool padding = false) {
    return           0b10000000
        | (padding ? 0b00100000 : 0)
        | (rc      & 0b00011111);
}

inline uint16_t LengthToWordsMinusOne(size_t length) {
    return static_cast<uint16_t>((length + 3) / 4 - 1);
}

}
