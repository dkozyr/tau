#pragma once

#include <ostream>

namespace tau::ice {

enum State {
    kWaiting,
    kRunning,
    kReady,     // on first valid pair
    kCompleted,
    kFailed
};

inline std::ostream& operator<<(std::ostream& s, const State& x) {
    switch(x) {
        case State::kWaiting:   return s << "waiting";
        case State::kRunning:   return s << "running";
        case State::kReady:     return s << "ready";
        case State::kCompleted: return s << "completed";
        case State::kFailed:    return s << "failed";
    }
    return s << "unknown";
}

}
