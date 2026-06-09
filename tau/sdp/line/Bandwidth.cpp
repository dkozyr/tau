#include "tau/sdp/line/Bandwidth.h"
#include "tau/common/String.h"
#include <etl/string_stream.h>

namespace tau::sdp {

etl::string_view BandwidthReader::GetType(const etl::string_view& value) {
    const auto tokens = Split(value, ":");
    return tokens[0];
}

size_t BandwidthReader::GetKbps(const etl::string_view& value) {
    const auto tokens = Split(value, ":");
    return StringToUnsigned(tokens[1]).value();
}

bool BandwidthReader::Validate(const etl::string_view& value) {
    const auto tokens = Split(value, ":");
    if(tokens.size() != 2) { return false; }
    const auto& bwtype = tokens[0];
    if(bwtype.size() < 2) { return false; }
    if(!((bwtype == "CT") || (bwtype == "AS") || (bwtype.substr(0, 2) == "X-"))) {
        return false;
    }
    return StringToUnsigned(tokens[1]).has_value();
}

etl::string_stream& BandwidthWriter::Write(etl::string_stream& ss, etl::string_view type, size_t kbps) {
    ss << type << ":" << kbps;
    return ss;
}

}
