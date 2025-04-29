#include "tau/ice/Candidate.h"
#include "tau/sdp/line/attribute/Candidate.h"
#include <sstream>

namespace tau::ice {

bool Candidate::operator<(const Candidate& other) const { 
    return priority > other.priority;
}

std::string ToCandidateAttributeString(CandidateType type, size_t socket_idx, Endpoint endpoint, std::string_view mdns_name) {
    return sdp::attribute::CandidateWriter::Write(
        Foundation(type, socket_idx), 1, "udp", Priority(type, socket_idx),
        mdns_name.empty() ? endpoint.address().to_string() : mdns_name,
        endpoint.port(), CandidateTypeToString(type), {});
}

CandidateType CandidateTypeFromString(const std::string_view& type) {
    if(type == "host")  { return CandidateType::kHost; }
    if(type == "prflx") { return CandidateType::kPeerRefl; }
    if(type == "srflx") { return CandidateType::kServRefl; }
    return CandidateType::kRelayed;
}

std::string CandidateTypeToString(CandidateType type) {
    switch(type) {
        case CandidateType::kHost:     return "host";
        case CandidateType::kPeerRefl: return "prflx";
        case CandidateType::kServRefl: return "srflx";
        case CandidateType::kRelayed:  return "relay";
    };
    return {};
}

// https://www.rfc-editor.org/rfc/rfc5245.html#section-4.1.2.1
uint32_t Priority(CandidateType type, size_t socket_idx) {
    uint32_t type_preference = static_cast<uint32_t>(type);
    uint32_t local_preference = 65535u - static_cast<uint32_t>(socket_idx);
    constexpr uint32_t component_preference = 255; // rtcp-mux only, so component ID == 1
    return (type_preference << 24) | (local_preference << 8) | component_preference;
}

// https://www.rfc-editor.org/rfc/rfc8445.html#section-5.1.1.3
uint32_t Foundation(CandidateType type, size_t socket_idx) {
    auto foundation = static_cast<uint32_t>(socket_idx << 8) | static_cast<uint32_t>(type);
    return (foundation << 1) | 1;
}

}
