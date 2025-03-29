#include "tau/ice/CandidatePair.h"

namespace tau::ice {

bool CandidatePair::operator<(const CandidatePair& other) const {
    if(state == other.state) {
        return priority > other.priority;
    } else {
        if(state == State::kFailed) { return false; }
        if(other.state == State::kFailed) { return true; }
        return state > other.state;
    }
}

// https://www.rfc-editor.org/rfc/rfc8445.html#section-6.1.2.3
uint64_t PairPriority(uint32_t priority_controlling, uint32_t priority_controlled) {
    uint64_t min = std::min(priority_controlling, priority_controlled);
    uint64_t max = std::max(priority_controlling, priority_controlled);
    return (min << 32) + (max << 1) + (priority_controlling > priority_controlled ? 1 : 0);
}

uint64_t PairPriority(Role role, uint32_t local_priority, uint32_t remote_priority) {
    if(role == Role::kControlling) {
        return PairPriority(local_priority, remote_priority);
    } else {
        return PairPriority(remote_priority, local_priority);
    }
}

}
