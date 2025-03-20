#include "tau/sdp/line/Connection.h"
#include "tau/common/String.h"

namespace tau::sdp {

std::string_view ConnectionReader::GetAddressType(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[1];
}

std::string_view ConnectionReader::GetAddress(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[2];
}

bool ConnectionReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() != 3) { return false; }
    if(tokens[0] != "IN") { return false; }
    if(!((tokens[1] == "IP4") || (tokens[1] == "IP6"))) { return false; }
    return true;
}

std::string ConnectionWriter::Write(std::string_view ip_addr, std::string_view addr_type) {
    std::stringstream ss;
    ss << "IN " << addr_type << " " << ip_addr;
    return ss.str();
}

}
