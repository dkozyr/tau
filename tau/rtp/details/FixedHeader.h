#pragma once

#include <cstdint>

namespace rtp::detail {

// Fixed header first byte only. https://datatracker.ietf.org/doc/html/rfc3550#section-5.1
struct FixedHeader {
    uint8_t cc : 4;
    uint8_t x : 1;
    uint8_t p : 1;
    uint8_t v : 2;
};

inline uint8_t BuildFixedHeader(bool extension = false, bool padding = false, uint8_t csrc_count = 0) {
    return              0b10000000
        | (padding    ? 0b00100000 : 0)
        | (extension  ? 0b00010000 : 0)
        | (csrc_count & 0b00001111);
}

}
