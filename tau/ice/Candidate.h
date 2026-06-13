#pragma once

#include "tau/net/Endpoint.h"
#include <etl/vector.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace tau::ice {

using IpAddress = net::IpAddress;
using Endpoint = net::Endpoint;

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
using Candidates = etl::vector<Candidate, 8>;
using CandidateStr = etl::string<64>;

CandidateStr ToCandidateAttributeString(CandidateType type, size_t socket_idx, Endpoint endpoint, etl::string_view mdns_name = {});
CandidateType CandidateTypeFromString(const etl::string_view& type);
etl::string_view CandidateTypeToString(CandidateType type);

uint32_t Priority(CandidateType type, size_t socket_idx);
uint32_t Foundation(CandidateType type, size_t socket_idx);

}
