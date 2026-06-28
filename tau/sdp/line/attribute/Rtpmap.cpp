#include "tau/sdp/line/attribute/Rtpmap.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {
    
uint8_t RtpmapReader::GetPt(const etl::string_view& value) {
    SplitTokens<1> tokens;
    Split(tokens, value, " ");
    return StringToUnsigned<uint8_t>(tokens[0]).value();
}

etl::string_view RtpmapReader::GetEncodingName(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    SplitTokens<2> name_clock;
    Split(name_clock, tokens[1], "/");
    return name_clock[0];
}

size_t RtpmapReader::GetClockRate(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    SplitTokens<2> name_clock;
    Split(name_clock, tokens[1], "/");
    return StringToUnsigned<uint32_t>(name_clock[1]).value();
}

etl::string_view RtpmapReader::GetParams(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    SplitTokens<3> params;
    Split(params, tokens[1], "/");
    if(params.size() == 3) {
        return params[2];
    }
    return {};
}

bool RtpmapReader::Validate(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    if(tokens.size() < 2) {
        return false;
    }
    const auto pt = StringToUnsigned<uint8_t>(tokens[0]);
    if(!pt || (*pt > 127)) {
        return false;
    }
    SplitTokens<2> name_clock;
    Split(name_clock, tokens[1], "/");
    if(name_clock.size() < 2) {
        return false;
    }
    const auto clock_rate = StringToUnsigned<uint32_t>(name_clock[1]);
    if(!clock_rate) {
        return false;
    }
    return true;
}

etl::string_stream& RtpmapWriter::Write(etl::string_stream& ss, uint8_t pt, etl::string_view encoding_name, size_t clock_rate, etl::string_view params) {
    ss << (size_t)pt << " " << encoding_name << "/" << clock_rate;
    if(!params.empty()) {
        ss << "/" << params;
    }
    return ss;

}

}
