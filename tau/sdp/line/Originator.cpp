#include "tau/sdp/line/Originator.h"
#include "tau/common/String.h"
#include "tau/common/Ntp.h"
#include "tau/common/Clock.h"

namespace tau::sdp {

std::string_view OriginatorReader::GetAddressType(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[4];
}

std::string_view OriginatorReader::GetAddress(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[5];
}

bool OriginatorReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() != 6) { return false; }
    if(tokens[3] != "IN") { return false; }
    if(!((tokens[4] == "IP4") || (tokens[4] == "IP6"))) { return false; }
    return true;
}

std::string OriginatorWriter::Write(std::string_view addr_type, std::string_view ip_addr) {
    std::stringstream ss;
    ss << "- " << ToNtp(SystemClock{}.Now()) << " 1 IN " << addr_type << " " << ip_addr;
    return ss.str();
}

}
