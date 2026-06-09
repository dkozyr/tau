#include <tau/net/host/Interface.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

namespace tau::net {

Interfaces EnumerateInterfaces(bool skip_loopback, bool ipv6) {
    ifaddrs* ifs;
    if(getifaddrs(&ifs)) {
        return {};
    }
    Interfaces result;
    for(auto addr = ifs; addr != nullptr; addr = addr->ifa_next) {
        if(addr->ifa_addr == nullptr) continue;
        if(!(addr->ifa_flags & IFF_UP)) continue;

        if(addr->ifa_addr->sa_family == AF_INET) {
            auto addr_v4 = reinterpret_cast<sockaddr_in*>(addr->ifa_addr)->sin_addr.s_addr;
            auto address = MakeIpAddressV4(addr_v4, true);
            if(skip_loopback && address.IsLoopback()) {
                continue;
            }
            result.push_back(Interface{
                .name = addr->ifa_name,
                .address = address
            });
        } else if(ipv6 && (addr->ifa_addr->sa_family == AF_INET6)) {
            // auto addr_v6 = reinterpret_cast<sockaddr_in6*>(addr->ifa_addr);
            // IpAddressV6::bytes_type buffer;
            // memcpy(buffer.data(), addr_v6->sin6_addr.s6_addr, sizeof(addr_v6->sin6_addr));
            // auto address = MakeIpAddressV6(buffer, addr_v6->sin6_scope_id);
            // if(skip_loopback && address.is_loopback()) {
            //     continue;
            // }
            // result.push_back(Interface{
            //     .name = addr->ifa_name,
            //     .address = address
            // });
        }
    }
    freeifaddrs(ifs);
    return result;
}

}
