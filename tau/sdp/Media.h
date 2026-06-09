#pragma once

#include "tau/sdp/MediaType.h"
#include "tau/sdp/Direction.h"
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>
#include <etl/unordered_map.h>
#include <optional>
#include <initializer_list>

namespace tau::sdp {

inline constexpr uint8_t kMaxCodecs = 32;

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
    etl::string<16> name = {};
    uint32_t clock_rate = 0;
    uint8_t rtcp_fb = 0;
    etl::string<256> format = {};
};

using CodecsMap = etl::unordered_map<uint8_t, Codec, kMaxCodecs>;

struct Media {
    MediaType type;
    etl::string<32> mid = {};
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
