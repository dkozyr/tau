#pragma once

#include "tau/rtsp/Response.h"
#include <sstream>

namespace rtsp {

class ResponseWriter {
public:
    static std::string Write(const Response& response) {
        std::stringstream ss;
        ss << kRtspVersion << " " << response.status_code << " " << response.reason_phrase << kClRf;
        for(auto& header : response.headers) {
            switch(header.name) {
                case HeaderName::kCSeq:          ss << "CSeq"; break;
                case HeaderName::kTransport:     ss << "Transport"; break;
                case HeaderName::kSession:       ss << "Session"; break;
                case HeaderName::kRtpInfo:       ss << "RTP-Info"; break;
                case HeaderName::kPublic:        ss << "Public"; break;
                case HeaderName::kContentBase:   ss << "Content-Base"; break;
                case HeaderName::kContentType:   ss << "Content-Type"; break;
                case HeaderName::kContentLength: ss << "Content-Length"; break;
                default:
                    continue;
            }
            ss << ": " << header.value << kClRf;
        }
        ss << "User-Agent: " << kUserAgent << kClRf
           << kClRf;
        if(!response.body.empty()) {
            ss << response.body;
        }
        return ss.str();
    }
};

}
