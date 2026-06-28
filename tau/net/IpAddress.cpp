#include <tau/net/IpAddress.h>
#include <tau/common/String.h>

namespace tau::net {

IpAddress MakeIpAddressV4(const etl::string_view& address_str) {
    etl::vector<etl::string_view, 4 + 1> tokens;
    Split(tokens, address_str, ".");
    if(tokens.size() == 4) {
        IpAddress ip;
        for(size_t i = 0; i < 4 + 1; ++i) {
            if(i == 4) {
                return ip;
            }
            const auto parsed = StringToUnsigned(tokens[i]);
            if(parsed && (*parsed <= std::numeric_limits<uint8_t>::max())) {
                ip.bytes[i] = static_cast<uint8_t>(*parsed);
            } else {
                break;
            }
        }
    }
    return IpAddress{};
}

IpAddressStrV4 ToString(const IpAddress& address) {
    IpAddressStrV4 result;
    etl::string_stream ss(result);
    ToString(ss, address);
    return result;
}

etl::string_stream& ToString(etl::string_stream& ss, const IpAddress& address) {
    ss  << static_cast<size_t>(address.bytes[0]) << "."
        << static_cast<size_t>(address.bytes[1]) << "."
        << static_cast<size_t>(address.bytes[2]) << "."
        << static_cast<size_t>(address.bytes[3]);
    return ss;
}

etl::string_stream& operator<<(etl::string_stream& ss, const IpAddress& address) {
    return ss << ToString(address);
}

}
