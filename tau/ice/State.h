#pragma once

#include <etl/string_stream.h>

namespace tau::ice {

enum State {
    kWaiting,
    kRunning,
    kReady,     // on first valid pair
    kCompleted,
    kFailed
};

inline etl::string_stream& operator<<(etl::string_stream& ss, const State& x) {
    switch(x) {
        case State::kWaiting:   return ss << "waiting";
        case State::kRunning:   return ss << "running";
        case State::kReady:     return ss << "ready";
        case State::kCompleted: return ss << "completed";
        case State::kFailed:    return ss << "failed";
    }
    return ss << "unknown";
}

}
