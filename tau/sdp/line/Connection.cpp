#include "tau/sdp/line/Connection.h"
#include "tau/common/String.h"
#include <etl/string_stream.h>

namespace tau::sdp {

etl::string_view ConnectionReader::GetAddressType(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[1];
}

etl::string_view ConnectionReader::GetAddress(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[2];
}

bool ConnectionReader::Validate(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() != 3) { return false; }
    if(tokens[0] != "IN") { return false; }
    if(!((tokens[1] == "IP4") || (tokens[1] == "IP6"))) { return false; }
    return true;
}

etl::string_stream& ConnectionWriter::Write(etl::string_stream& ss, etl::string_view ip_addr, etl::string_view addr_type) {
    ss << "IN " << addr_type << " " << ip_addr;
    return ss;
}

}
