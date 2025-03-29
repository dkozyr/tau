#pragma once

#include "tau/ice/Candidate.h"
#include "tau/ice/Role.h"

namespace tau::ice {

struct CandidatePair {
    enum State {
        kFrozen     = 1,
        kWaiting    = 2,
        kInProgress = 3,
        kSucceeded  = 4,
        kNominating = 5,
        kNominated  = 6,
        kFailed     = 99,
    };

    size_t id;
    Candidate local;
    Candidate remote;
    uint64_t priority;
    State state = State::kFrozen;
    size_t attempts_count = 0;

    bool operator<(const CandidatePair& other) const;
};
using CandidatePairs = std::vector<CandidatePair>;

uint64_t PairPriority(Role role, uint32_t local_priority, uint32_t remote_priority);

}
