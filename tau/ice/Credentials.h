#pragma once

#include "tau/crypto/Md5.h"
#include <etl/string_view.h>
#include <cstdint>

namespace tau::ice {

inline constexpr size_t kLongTermPassword = crypto::kMd5DigestLength;

struct PeerCredentials {
    etl::string_view ufrag;
    etl::string_view password;
};

struct Credentials {
    PeerCredentials local;
    PeerCredentials remote;
};

bool CalcLongTermPassword(const PeerCredentials& credentials, etl::string_view realm, uint8_t* output);

}
