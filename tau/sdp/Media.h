#pragma once

#include "tau/sdp/MediaType.h"
#include "tau/sdp/Direction.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace tau::sdp {

enum RtcpFb : uint8_t {
    kNack = 1,
    kPli  = 2,
    kFir  = 4,
    // kTwcc = 8, //TODO: impl transport-cc
};

struct Codec {
    size_t index;
    std::string name = {};
    uint32_t clock_rate = 0;
    uint8_t rtcp_fb = 0;
    std::string format = {};
};

struct Media {
    MediaType type;
    Direction direction = Direction::kSendRecv;
    std::unordered_map<uint8_t, Codec> codecs = {};
};

}
