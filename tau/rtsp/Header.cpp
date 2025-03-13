#include "tau/rtsp/Header.h"
#include "tau/common/String.h"

namespace rtsp {

const std::vector<Header> kNameToPrefix = {
    Header{.name = HeaderName::kPublic,        .value = "Public: "},
    Header{.name = HeaderName::kAccept,        .value = "Accept: "},
    Header{.name = HeaderName::kTransport,     .value = "Transport: "},
    Header{.name = HeaderName::kSession,       .value = "Session: "},
    Header{.name = HeaderName::kRtpInfo,       .value = "RTP-Info: "},
    Header{.name = HeaderName::kContentBase,   .value = "Content-Base: "},
    Header{.name = HeaderName::kContentType,   .value = "Content-Type: "},
    Header{.name = HeaderName::kContentLength, .value = "Content-Length: "},
};

std::vector<Header> GetHeaders(const std::vector<std::string_view>& lines) {
    std::vector<Header> headers;
    for(size_t i = 2; i < lines.size(); ++i) { // skip first two lines
        const auto& line = lines[i];
        if(line == kClRf) {
            break;
        }
        for(auto& [name, prefix] : kNameToPrefix) {
            auto value = GetHeaderValue(line, prefix);
            if(!value.empty()) {
                headers.push_back(Header{
                    .name = name,
                    .value = std::string{value}
                });
                break;
            }
        }
    }
    return headers;
}

std::string_view GetHeaderValue(std::string_view line, std::string_view prefix) {
    if(IsPrefix(line, prefix)) {
        return line.substr(prefix.size());
    }
    return {};
}

}
