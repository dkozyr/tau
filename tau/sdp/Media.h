#pragma once

#include "tau/sdp/MediaType.h"
#include "tau/sdp/MediaCodec.h"
#include "tau/sdp/Direction.h"
#include "tau/sdp/Mid.h"
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>
#include <etl/unordered_map.h>
#include <optional>
#include <initializer_list>

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
    using Name = etl::string<16>;
    using Format = etl::string<128>;

    size_t index;
    Name name = {};
    uint32_t clock_rate = 0;
    uint8_t rtcp_fb = 0;
    Format format = {};
};

using CodecsMap = etl::unordered_map<uint8_t, Codec, kMaxCodecs>;

struct Media {
    MediaType type;
    Mid mid = {};
    Direction direction = Direction::kSendRecv;
    CodecsMap codecs = {};
    std::optional<uint32_t> ssrc = std::nullopt; //NOTE: should be revised for the case of several streams: video and rtx
};

using Medias = etl::vector<Media, 3>;

struct PtWithPriority {
    uint8_t pt;
    size_t index;
};
etl::vector<uint8_t, kMaxCodecs> GetPtOrdered(const CodecsMap& codecs);
etl::vector<PtWithPriority, kMaxCodecs> GetPtWithPriority(const CodecsMap& codecs);

CodecsMap MakeCodecsMap(std::initializer_list<std::pair<const uint8_t, Codec>> list);

}
