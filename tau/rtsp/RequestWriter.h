#pragma once

#include "tau/rtsp/Request.h"
#include <sstream>

namespace tau::rtsp {

class RequestWriter {
public:
    static std::string Write(const Request& request) {
        std::stringstream ss;
        switch(request.method) {
            case Method::kOptions:  ss << "OPTIONS "; break;
            case Method::kDescribe: ss << "DESCRIBE "; break;
            case Method::kSetup:    ss << "SETUP "; break;
            case Method::kPlay:     ss << "PLAY "; break;
            case Method::kTeardown: ss << "TEARDOWN "; break;
        }
        ss << (request.method == Method::kOptions ? "*" : request.uri) << " " << kRtspVersion << kClRf;
        for(auto& header : request.headers) {
            switch(header.name) {
                case HeaderName::kCSeq:      ss << "CSeq"; break;
                case HeaderName::kAccept:    ss << "Accept"; break;
                case HeaderName::kTransport: ss << "Transport"; break;
                case HeaderName::kSession:   ss << "Session"; break;
                default:
                    continue;
            }
            ss << ": " << header.value << kClRf;
        }
        ss << "User-Agent: " << kUserAgent << kClRf
           << kClRf;
        return ss.str();
    }
};

}
