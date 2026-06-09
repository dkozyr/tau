#pragma once

#include <etl/string_stream.h>
#include <cstdint>

namespace tau::webrtc {

enum State : uint8_t {
    kInitial    = 0,
    kConnecting = 1,
    // kReady      = 2,
    kConnected  = 3,
    kFailed     = 4,
};

inline etl::string_stream& operator<<(etl::string_stream& ss, const State& x) {
    switch(x) {
        case State::kInitial:    return ss << "initial";
        case State::kConnecting: return ss << "connecting";
        // case State::kReady:      return ss << "ready";
        case State::kConnected:  return ss << "connected";
        case State::kFailed:     return ss << "failed";
    }
    return ss << "unknown";
}

}
