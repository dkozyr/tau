#pragma once

#include <cstdint>
#include <cstddef>

namespace tau::rtp {

inline constexpr size_t kFixedHeaderSize = 3 * sizeof(uint32_t);
inline constexpr size_t kExtensionHeaderSize = sizeof(uint32_t);

inline constexpr size_t HeaderExtensionSize(uint16_t length_in_words) {
    return (length_in_words > 0)
        ? kExtensionHeaderSize + sizeof(uint32_t) * length_in_words
        : 0;
}

}
