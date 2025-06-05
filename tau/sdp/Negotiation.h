#pragma once

#include "tau/sdp/Sdp.h"

namespace tau::sdp {

// https://datatracker.ietf.org/doc/html/rfc6184#section-8.1
inline constexpr std::string_view kH264BaseProfileLevel1_0{"42000a"};

std::optional<Media> SelectMedia(const Media& remote, const Media& local);
Direction SelectDirection(Direction remote, Direction local);
uint8_t SelectRtcpFb(uint8_t remote, uint8_t local);

// H265 specific
CodecsMap FilterH265Codec(const CodecsMap& origin);

// H264 specific
CodecsMap FilterH264Codec(const CodecsMap& origin, bool asymmetry_allowed);
bool IsH264SupportAsymmetry(const CodecsMap& codecs);
bool IsH264SameProfile(const std::string& remote_format, const std::string& local_format);
std::string_view SelectH264Level(const std::string& remote_format, const std::string& local_format, bool asymmetry);
std::string_view GetH264ProfileLevelId(const std::string& codec_format);

}
