#include "tau/net/Resolver.h"
#include "tau/common/Log.h"

namespace tau::net {

std::optional<ResolverResults> Resolve(Executor executor, const std::string& host, uint16_t port) {
    Resolver resolver(executor);
    boost_ec ec;

    const auto service = std::to_string(port);
    const auto results = resolver.resolve(host, service, ec);
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.value() << ", message: " << ec.message());
        return std::nullopt;
    }
    return results;
}

}
