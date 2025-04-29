#pragma once

#include "tau/asio/Common.h"
#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include <cstddef>

namespace tau::ice {

enum CandidateType : uint8_t {
    kHost     = 126,
    kPeerRefl = 110,
    kServRefl = 100,
    kRelayed  = 0,
};

struct Candidate {
    CandidateType type;
    uint32_t priority;
    Endpoint endpoint;
    std::optional<size_t> socket_idx = {};

    bool operator<(const Candidate& other) const;
};
using Candidates = std::vector<Candidate>;

std::string ToCandidateAttributeString(CandidateType type, size_t socket_idx, Endpoint endpoint, std::string_view mdns_name = {});
CandidateType CandidateTypeFromString(const std::string_view& type);
std::string CandidateTypeToString(CandidateType type);

uint32_t Priority(CandidateType type, size_t socket_idx);
uint32_t Foundation(CandidateType type, size_t socket_idx);

}
