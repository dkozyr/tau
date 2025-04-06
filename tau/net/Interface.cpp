#include "tau/net/Interface.h"
#include <ifaddrs.h>

namespace tau::net {

std::vector<Interface> EnumerateInterfaces(bool skip_loopback, bool ipv6) {
    ifaddrs* ifs;
    if(getifaddrs(&ifs)) {
        return {};
    }
    std::vector<Interface> result;
    for(auto addr = ifs; addr != nullptr; addr = addr->ifa_next) {
        if(addr->ifa_addr == nullptr) continue;
        if(!(addr->ifa_flags & IFF_UP)) continue;

        if(addr->ifa_addr->sa_family == AF_INET) {
            auto addr_v4 = ntohl(reinterpret_cast<sockaddr_in*>(addr->ifa_addr)->sin_addr.s_addr);
            auto address = asio_ip::make_address_v4(addr_v4);
            if(skip_loopback && address.is_loopback()) {
                continue;
            }
            result.push_back(Interface{
                .name = addr->ifa_name,
                .address = address
            });
        } else if(ipv6 && (addr->ifa_addr->sa_family == AF_INET6)) {
            auto addr_v6 = reinterpret_cast<sockaddr_in6*>(addr->ifa_addr);
            IpAddressV6::bytes_type buffer;
            memcpy(buffer.data(), addr_v6->sin6_addr.s6_addr, sizeof(addr_v6->sin6_addr));
            auto address = asio_ip::make_address_v6(buffer, addr_v6->sin6_scope_id);
            if(skip_loopback && address.is_loopback()) {
                continue;
            }
            result.push_back(Interface{
                .name = addr->ifa_name,
                .address = address
            });
        }
    }
    freeifaddrs(ifs);
    return result;
}

}
