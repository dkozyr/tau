#pragma once

#include "tau/sdp/MediaType.h"
#include "tau/sdp/Direction.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace tau::sdp {

enum RtcpFb : uint8_t {
    kNone = 0,
    kNack = 1,
    kPli  = 2,
    kFir  = 4,
    // kTwcc = 8, //TODO: impl transport-cc
};
inline constexpr uint8_t kRtcpFbDefault = RtcpFb::kNack | RtcpFb::kPli | RtcpFb::kFir;

struct Codec {
    size_t index;
    std::string name = {};
    uint32_t clock_rate = 0;
    uint8_t rtcp_fb = 0;
    std::string format = {};
};

using CodecsMap = std::unordered_map<uint8_t, Codec>;

struct Media {
    MediaType type;
    std::string mid = {};
    Direction direction = Direction::kSendRecv;
    CodecsMap codecs = {};
    std::optional<uint32_t> ssrc = std::nullopt; //NOTE: should be revised for the case of several streams: video and rtx
};

struct PtWithPriority {
    uint8_t pt;
    size_t index;
};
std::vector<uint8_t> GetPtOrdered(const CodecsMap& codecs);
std::vector<PtWithPriority> GetPtWithPriority(const CodecsMap& codecs);

}
