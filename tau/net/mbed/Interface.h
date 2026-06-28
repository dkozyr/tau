#pragma once

#include <tau/net/IpAddress.h>
#include <etl/vector.h>
#include <etl/string.h>

namespace tau::net {

struct Interface {
    using Name = etl::string<16>;

    Name name;
    IpAddress address;
};

using Interfaces = etl::vector<Interface, 8>;

Interfaces EnumerateInterfaces(bool skip_loopback, bool ipv6 = false);

}
