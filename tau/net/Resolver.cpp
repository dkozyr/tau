#include "tau/net/Resolver.h"
#include "tau/common/Log.h"
#include <netdb.h>
#include <netinet/in.h>

namespace tau::net {

std::optional<Endpoint> ResolveEndpointV4(const etl::string_view& host, const etl::string_view& service) {
    addrinfo hints;
    addrinfo* result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    auto status = getaddrinfo(host.data(), service.data(), &hints, &result);
    if(status != 0) {
        TAU_LOG_ERROR("getaddrinfo error: " << status);
        return std::nullopt;
    }

    std::optional<Endpoint> endpoint;

    auto entry = result;
    while(entry) {
        if(entry->ai_family == AF_INET) {
            auto addr = reinterpret_cast<sockaddr_in*>(entry->ai_addr);
            auto addr_v4 = addr->sin_addr.s_addr;
            endpoint.emplace(Endpoint{
                .address = MakeIpAddressV4(addr_v4, true),
                .port = htons(addr->sin_port)
            });
            break;
        }
        entry = entry->ai_next;
    }
    freeaddrinfo(result);
    return endpoint;
}

}
