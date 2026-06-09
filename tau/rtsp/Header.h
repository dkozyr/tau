#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>
#include <cstdint>
#include <cstddef>

namespace tau::rtsp {

inline constexpr etl::string_view kRtspVersion = "RTSP/1.0";
inline constexpr etl::string_view kUserAgent = "tau";
inline constexpr etl::string_view kClRf = "\r\n";
inline constexpr etl::string_view kClRfClRf = "\r\n\r\n";

enum HeaderName : uint8_t {
    kCSeq = 0,
    kAccept,
    kTransport,
    kSession,
    kRtpInfo,
    kPublic,
    kContentBase,
    kContentType,
    kContentLength,
    kMaxIndex
};

struct Header {
    HeaderName name;
    etl::string_view value;
};
using Headers = etl::vector<Header, HeaderName::kMaxIndex>;

Headers GetHeaders(const etl::ivector<etl::string_view>& lines);
etl::string_view GetHeaderValue(etl::string_view line, etl::string_view prefix);
etl::string_view GetHeaderValue(HeaderName name, const Headers& headers);

}
