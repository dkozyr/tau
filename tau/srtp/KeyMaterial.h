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

inline size_t GetKeySize(srtp_profile_t profile) {
    return srtp_profile_get_master_key_length(profile);
}

inline size_t GetSaltSize(srtp_profile_t profile) {
    return srtp_profile_get_master_salt_length(profile);
}

}
