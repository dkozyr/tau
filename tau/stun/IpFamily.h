#pragma once

#include <cstdint>

namespace tau::stun {

enum class IpFamily : uint8_t {
    kIpv4 = 1,
    kIpv6 = 2,
};

}
