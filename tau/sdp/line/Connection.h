#pragma once

#include <string>
#include <string_view>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.7
class ConnectionReader {
public:
    static std::string_view GetAddressType(const std::string_view& value);
    static std::string_view GetAddress(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class ConnectionWriter {
public:
    static std::string Write(std::string_view ip_addr, std::string_view addr_type = "IP4");
};

}
