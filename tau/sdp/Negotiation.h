#pragma once

#include "tau/sdp/Sdp.h"

namespace tau::sdp {

// https://datatracker.ietf.org/doc/html/rfc6184#section-8.1
inline constexpr etl::string_view kH264BaseProfileLevel1_0{"42000a"};

std::optional<Media> SelectMedia(const Media& remote, const Media& local);
Direction SelectDirection(Direction remote, Direction local);
uint8_t SelectRtcpFb(uint8_t remote, uint8_t local);

// H265 specific
CodecsMap FilterH265Codec(const CodecsMap& origin);

// H264 specific
CodecsMap FilterH264Codec(const CodecsMap& origin, bool asymmetry_allowed);
bool IsH264SupportAsymmetry(const CodecsMap& codecs);
bool IsH264SameProfile(const etl::string_view& remote_format, const etl::string_view& local_format);
etl::string_view SelectH264Level(const etl::string_view& remote_format, const etl::string_view& local_format, bool asymmetry);
etl::string_view GetH264ProfileLevelId(const etl::string_view& codec_format);

}
