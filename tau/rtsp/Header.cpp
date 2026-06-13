#include "tau/rtsp/Header.h"
#include "tau/common/String.h"
#include <etl/array.h>

namespace tau::rtsp {

const etl::array<etl::string_view, kMaxIndex> kNameToPrefix = {
    etl::string_view{"CSeq: "},
    etl::string_view{"Accept: "},
    etl::string_view{"Transport: "},
    etl::string_view{"Session: "},
    etl::string_view{"RTP-Info: "},
    etl::string_view{"Public: "},
    etl::string_view{"Content-Base: "},
    etl::string_view{"Content-Type: "},
    etl::string_view{"Content-Length: "}
};

Headers GetHeaders(const etl::string_view& str) {
    Headers headers;
    size_t pos = 0;
    while(pos != etl::string_view::npos) {
        auto line = SplitNext(str, pos, kClRf);
        if((line == kClRf) || line.empty()) {
            break;
        }
        if(headers.full()) {
            break;
        }
        for(uint8_t name = HeaderName::kCSeq; name < HeaderName::kMaxIndex; ++name) {
            auto& prefix = kNameToPrefix[name];
            auto value = GetHeaderValue(line, prefix);
            if(!value.empty() && (headers.size() < headers.max_size())) {
                headers.push_back(Header{
                    .name = static_cast<HeaderName>(name),
                    .value = etl::string_view{value}
                });
                break;
            }
        }
    }
    return headers;
}

etl::string_view GetHeaderValue(etl::string_view line, etl::string_view prefix) {
    if(IsPrefix(line, prefix, true)) {
        return line.substr(prefix.size());
    }
    return {};
}

etl::string_view GetHeaderValue(HeaderName name, const Headers& headers) {
    for(auto& header : headers) {
        if(header.name == name) {
            return header.value;
        }
    }
    return {};
}

}
