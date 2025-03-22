#include "tau/sdp/line/attribute/Candidate.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {

std::string_view CandidateReader::GetFoundation(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[0];
}

uint16_t CandidateReader::GetComponentId(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint16_t>(tokens[1]).value();
}

std::string_view CandidateReader::GetTransport(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[2];
}

uint32_t CandidateReader::GetPriority(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint32_t>(tokens[3]).value();
}

std::string_view CandidateReader::GetAddress(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[4];
}

uint16_t CandidateReader::GetPort(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint16_t>(tokens[5]).value();
}

std::string_view CandidateReader::GetType(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[7];
}

std::string_view CandidateReader::GetExtParameters(const std::string_view& value) {
    size_t pos = 0;
    for(size_t i = 0; i < 8; ++i) {
        pos = value.find(' ', pos);
        if(pos == std::string::npos) {
            return {};
        }
        pos++;
    }
    return value.substr(pos);
}

bool CandidateReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() < 8)                      { return false; }
    if(tokens[6] != "typ")                     { return false; }
    if(!StringToUnsigned<uint16_t>(tokens[1])) { return false; }
    if(!StringToUnsigned<uint32_t>(tokens[3])) { return false; }
    if(!StringToUnsigned<uint16_t>(tokens[5])) { return false; }
    if(tokens[7].empty())                      { return false; }
    return true;
}


std::string CandidateWriter::Write(std::string_view foundation, uint16_t component_id,
        std::string_view transport, uint32_t priority, std::string_view address,
        uint16_t port, std::string_view type, std::string_view ext_parameters) {
    std::stringstream ss;
    ss << foundation << " " << component_id << " " << transport << " " << priority
       << " " << address << " " << port << " typ " << type << " " << ext_parameters;
    return ss.str();
}

}
