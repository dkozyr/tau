#pragma once

#include <string>
#include <string_view>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.2
class OriginatorReader {
public:
    static std::string_view GetAddressType(const std::string_view& value);
    static std::string_view GetAddress(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class OriginatorWriter {
public:
    static std::string Write(std::string_view addr_type, std::string_view ip_addr);
};

}
