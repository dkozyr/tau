#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace tau::ice {

inline constexpr size_t kLongTermPassword = 16; // MD5 size

struct PeerCredentials {
    std::string ufrag;
    std::string password;
};

struct Credentials {
    PeerCredentials local;
    PeerCredentials remote;
};

bool CalcLongTermPassword(const PeerCredentials& credentials, std::string_view realm, uint8_t* output);

}
