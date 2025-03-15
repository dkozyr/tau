#include "tau/rtsp/Response.h"
#include "tau/memory/Buffer.h"
#include "tau/rtcp/Reader.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"

namespace tau {

inline std::optional<uint16_t> ParseServerRtpPort(const rtsp::Response& response) {
    constexpr std::string_view kServerPortStr = "server_port=";
    for(auto& header : response.headers) {
        if(header.name == rtsp::HeaderName::kTransport) {
            auto tokens = Split(header.value, ";");
            for(auto& token : tokens) {
                if(IsPrefix(token, kServerPortStr)) {
                    return StringToUnsigned<uint16_t>(token.substr(kServerPortStr.size()));
                }
            }
            break;
        }
    }
    return std::nullopt;
}

inline std::string_view ParseSessionId(const rtsp::Response& response) {
    for(auto& header : response.headers) {
        if(header.name == rtsp::HeaderName::kSession) {
            auto tokens = Split(header.value, ";");
            if(!tokens.empty()) {
                return tokens[0];
            }
            break;
        }
    }
    return {};
}

inline void PrintRtcp(const Buffer& packet) {
    const auto view = packet.GetView();
    if(!rtcp::Reader::Validate(view)) {
        LOG_WARNING << "Invalid RTCP, size: " << view.size;
        return;
    };
    rtcp::Reader::ForEachReport(view, [](rtcp::Type type, BufferViewConst report) {
        LOG_INFO << "Rtcp report type: " << (size_t)type << ", size: " << report.size;
        return true;
    });
}

}
