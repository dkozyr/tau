#pragma once

#include <etl/string_stream.h>
#include <cstdint>

namespace tau::signalling {

using DeviceId = uint64_t;
using StreamId = uint64_t;
using ClientId = uint64_t;
using SessionId = uint64_t;

enum class SessionState {
    kUnknown, //TODO: do we need it?
    kWait,
    kSdpOffered,
    kSdpAnswered,
    kStreaming,
    kClosed //TODO: do we need it?
};

inline SessionState SessionStateFromString(const etl::string_view& str) {
    if(str == "wait")         { return SessionState::kWait; }
    if(str == "sdp_offered")  { return SessionState::kSdpOffered; }
    if(str == "sdp_answered") { return SessionState::kSdpAnswered; }
    if(str == "streaming")    { return SessionState::kStreaming; }
    if(str == "closed")       { return SessionState::kClosed; }
    return SessionState::kUnknown;
}

inline etl::string_stream& operator<<(etl::string_stream& ss, SessionState state) {
    switch(state) {
        case SessionState::kUnknown:     return ss << "unknown";
        case SessionState::kWait:        return ss << "wait";
        case SessionState::kSdpOffered:  return ss << "sdp_offered";
        case SessionState::kSdpAnswered: return ss << "sdp_answered";
        case SessionState::kStreaming:   return ss << "streaming";
        case SessionState::kClosed:      return ss << "closed";
    }
    return ss << "unknown";
}

}
