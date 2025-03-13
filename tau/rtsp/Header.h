#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace rtsp {

inline constexpr std::string_view kRtspVersion = "RTSP/1.0";
inline constexpr std::string_view kUserAgent = "tau";
inline constexpr std::string_view kClRf = "\r\n";
inline constexpr std::string_view kClRfClRf = "\r\n\r\n";

enum HeaderName {
    kAccept,
    kTransport,
    kSession,
    kRtpInfo,
    kPublic,
    kContentBase,
    kContentType,
    kContentLength,
};

struct Header {
    HeaderName name;
    std::string value;
};

std::vector<Header> GetHeaders(const std::vector<std::string_view>& lines);
std::string_view GetHeaderValue(std::string_view line, std::string_view prefix);

}
