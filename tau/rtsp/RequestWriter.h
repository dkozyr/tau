#pragma once

#include "tau/rtsp/Request.h"
#include <etl/string_stream.h>

namespace tau::rtsp {

class RequestWriter {
public:
    static etl::istring& Write(const Request& request, etl::istring& buffer) {
        buffer.clear();
        etl::string_stream ss(buffer);
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
        buffer.resize(ss.str().size());
        return buffer;
    }
};

}
