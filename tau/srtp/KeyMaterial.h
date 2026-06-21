#pragma once

#include <srtp3/srtp.h>
#include <etl/vector.h>
#include <cstdint>

namespace tau::srtp {

inline constexpr auto kKeyCapacity = SRTP_MAX_KEY_LEN;
inline constexpr auto kSaltCapacity = SRTP_SALT_LEN;

struct KeyMaterial {
    etl::vector<uint8_t, kKeyCapacity> key;
    etl::vector<uint8_t, kSaltCapacity> salt;
};

}
