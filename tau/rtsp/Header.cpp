#include "tau/rtsp/Header.h"
#include "tau/common/String.h"

namespace tau::rtsp {

const std::vector<Header> kNameToPrefix = {
    Header{.name = HeaderName::kCSeq,          .value = "CSeq: "},
    Header{.name = HeaderName::kPublic,        .value = "Public: "},
    Header{.name = HeaderName::kAccept,        .value = "Accept: "},
    Header{.name = HeaderName::kTransport,     .value = "Transport: "},
    Header{.name = HeaderName::kSession,       .value = "Session: "},
    Header{.name = HeaderName::kRtpInfo,       .value = "RTP-Info: "},
    Header{.name = HeaderName::kContentBase,   .value = "Content-Base: "},
    Header{.name = HeaderName::kContentType,   .value = "Content-Type: "},
    Header{.name = HeaderName::kContentLength, .value = "Content-Length: "},
};

Headers GetHeaders(const std::vector<std::string_view>& lines) {
    Headers headers;
    for(size_t i = 1; i < lines.size(); ++i) { // skip first line
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
    if(IsPrefix(line, prefix, true)) {
        return line.substr(prefix.size());
    }
    return {};
}

std::string_view GetHeaderValue(HeaderName name, const Headers& headers) {
    for(auto& header : headers) {
        if(header.name == name) {
            return header.value;
        }
    }
    return {};
}

}
