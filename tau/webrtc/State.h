#pragma once

#include <cstdint>
#include <ostream>

namespace tau::webrtc {

enum State : uint8_t {
    kInitial    = 0,
    kConnecting = 1,
    // kReady      = 2,
    kConnected  = 3,
    kFailed     = 4,
};

inline std::ostream& operator<<(std::ostream& s, const State& x) {
    switch(x) {
        case State::kInitial:    return s << "initial";
        case State::kConnecting: return s << "connecting";
        // case State::kReady:      return s << "ready";
        case State::kConnected:  return s << "connected";
        case State::kFailed:     return s << "failed";
    }
    return s << "unknown";
}

}
