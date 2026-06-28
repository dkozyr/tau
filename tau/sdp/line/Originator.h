#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.2
class OriginatorReader {
public:
    static etl::string_view GetAddressType(const etl::string_view& value);
    static etl::string_view GetAddress(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class OriginatorWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, etl::string_view addr_type, etl::string_view ip_addr);
};

}
