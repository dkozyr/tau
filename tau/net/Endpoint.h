#pragma once

#include <tau/net/IpAddress.h>

namespace tau::net {

using EndpointStrV4 = etl::string<21>; //xxx.xxx.xxx.xxx:xxxxx

//TODO: support ipv6
struct Endpoint {
    IpAddress address;
    uint16_t port;

    bool operator==(const Endpoint& other) const;
    bool operator!=(const Endpoint& other) const;
};

EndpointStrV4 ToString(const Endpoint& endpoint);
etl::string_stream& ToString(etl::string_stream& ss, const Endpoint& endpoint);
etl::string_stream& operator<<(etl::string_stream& ss, const Endpoint& endpoint);

}

namespace tau {

using Endpoint = net::Endpoint;
using EndpointStrV4 = net::EndpointStrV4;

}

namespace etl {

template <>
struct hash<tau::net::Endpoint> {
    size_t operator()(const tau::net::Endpoint& endpoint) const noexcept {
        return static_cast<size_t>(endpoint.address.GetUint32()) ^ (static_cast<size_t>(endpoint.port) << 8);
    }
};

}
