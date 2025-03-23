#pragma once

#include "tau/asio/Common.h"
#include <vector>
#include <string>

namespace tau::net {

struct Interface {
    std::string name;
    asio_ip::address address;
};

std::vector<Interface> EnumerateInterfaces(bool skip_loopback, bool ipv6 = false);

}
