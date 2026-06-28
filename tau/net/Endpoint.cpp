#include <tau/net/Endpoint.h>

namespace tau::net {

bool Endpoint::operator==(const Endpoint& other) const { 
    return (address == other.address) && (port == other.port);
}

bool Endpoint::operator!=(const Endpoint& other) const { 
    return (address != other.address) || (port != other.port);
}

EndpointStrV4 ToString(const Endpoint& endpoint) {
    EndpointStrV4 result;
    etl::string_stream ss(result);
    ToString(ss, endpoint);
    return result;
}

etl::string_stream& ToString(etl::string_stream& ss, const Endpoint& endpoint) {
    return ss << endpoint.address << ":" << endpoint.port;
}

etl::string_stream& operator<<(etl::string_stream& ss, const Endpoint& endpoint) {
    return ss << endpoint.address << ":" << endpoint.port;
}

}
