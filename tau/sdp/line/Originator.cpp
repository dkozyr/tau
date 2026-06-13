#include "tau/sdp/line/Originator.h"
#include "tau/common/String.h"
#include "tau/common/Ntp.h"
#include "tau/common/SteadyClock.h" //TODO: can we use SystemClock?

namespace tau::sdp {

etl::string_view OriginatorReader::GetAddressType(const etl::string_view& value) {
    SplitTokens<5> tokens;
    Split(tokens, value, " ");
    return tokens[4];
}

etl::string_view OriginatorReader::GetAddress(const etl::string_view& value) {
    SplitTokens<6> tokens;
    Split(tokens, value, " ");
    return tokens[5];
}

bool OriginatorReader::Validate(const etl::string_view& value) {
    SplitTokens<6> tokens;
    Split(tokens, value, " ");
    if(tokens.size() != 6) { return false; }
    if(tokens[3] != "IN") { return false; }
    if(!((tokens[4] == "IP4") || (tokens[4] == "IP6"))) { return false; }
    return true;
}

etl::string_stream& OriginatorWriter::Write(etl::string_stream& ss, etl::string_view addr_type, etl::string_view ip_addr) {
    ss << "- " << ToNtp(SteadyClock{}.Now()) << " 1 IN " << addr_type << " " << ip_addr;
    return ss;
}

}
