#include "tau/sdp/line/attribute/Candidate.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {

etl::string_view CandidateReader::GetFoundation(const etl::string_view& value) {
    SplitTokens<1> tokens;
    Split(tokens, value, " ");
    return tokens[0];
}

uint16_t CandidateReader::GetComponentId(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    return StringToUnsigned<uint16_t>(tokens[1]).value();
}

etl::string_view CandidateReader::GetTransport(const etl::string_view& value) {
    SplitTokens<3> tokens;
    Split(tokens, value, " ");
    return tokens[2];
}

uint32_t CandidateReader::GetPriority(const etl::string_view& value) {
    SplitTokens<4> tokens;
    Split(tokens, value, " ");
    return StringToUnsigned<uint32_t>(tokens[3]).value();
}

etl::string_view CandidateReader::GetAddress(const etl::string_view& value) {
    SplitTokens<5> tokens;
    Split(tokens, value, " ");
    return tokens[4];
}

uint16_t CandidateReader::GetPort(const etl::string_view& value) {
    SplitTokens<6> tokens;
    Split(tokens, value, " ");
    return StringToUnsigned<uint16_t>(tokens[5]).value();
}

etl::string_view CandidateReader::GetType(const etl::string_view& value) {
    SplitTokens<8> tokens;
    Split(tokens, value, " ");
    return tokens[7];
}

etl::string_view CandidateReader::GetExtParameters(const etl::string_view& value) {
    size_t pos = 0;
    for(size_t i = 0; i < 8; ++i) {
        pos = value.find(' ', pos);
        if(pos == etl::string_view::npos) {
            return {};
        }
        pos++;
    }
    return value.substr(pos);
}

bool CandidateReader::Validate(const etl::string_view& value) {
    SplitTokens<8> tokens;
    Split(tokens, value, " ");
    if(tokens.size() < 8)                      { return false; }
    if(tokens[6] != "typ")                     { return false; }
    if(!StringToUnsigned<uint16_t>(tokens[1])) { return false; }
    if(!StringToUnsigned<uint32_t>(tokens[3])) { return false; }
    if(!StringToUnsigned<uint16_t>(tokens[5])) { return false; }
    if(tokens[7].empty())                      { return false; }
    return true;
}


etl::string_stream& CandidateWriter::Write(etl::string_stream& ss,
        uint32_t foundation, uint16_t component_id,
        etl::string_view transport, uint32_t priority, etl::string_view address,
        uint16_t port, etl::string_view type, etl::string_view ext_parameters) {
    ss << foundation << " " << component_id << " " << transport << " " << priority
       << " " << address << " " << port << " typ " << type;
    if(!ext_parameters.empty()) {
        ss << " " << ext_parameters;
    }
    return ss;
}

}
