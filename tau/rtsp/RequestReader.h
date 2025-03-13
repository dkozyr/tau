#pragma once

#include "tau/rtsp/Request.h"
#include "tau/common/String.h"
#include <optional>

namespace rtsp {

class RequestReader {
public:
    static std::optional<Request> Read(std::string_view str) {
        auto lines = Split(str, kClRf);
        if(lines.size() < 2) {
            return std::nullopt;
        }
        auto tokens = Split(lines[0], " ");
        if((tokens.size() != 3) || (tokens[2] != kRtspVersion)) {
            return std::nullopt;
        }
        auto method = GetMethod(tokens[0]);
        auto cseq = StringToUnsigned<size_t>(GetHeaderValue(lines[1], "CSeq: "));
        if(!method || !cseq) {
            return std::nullopt;
        }
        return Request{
            .uri = std::string{tokens[1]},
            .method = *method,
            .cseq = *cseq,
            .headers = GetHeaders(lines)
        };
    }

private:
    static std::optional<Method> GetMethod(std::string_view token) {
        if(token == "OPTIONS")  { return Method::kOptions; }
        if(token == "DESCRIBE") { return Method::kDescribe; }
        if(token == "SETUP")    { return Method::kSetup; }
        if(token == "PLAY")     { return Method::kPlay; }
        if(token == "TEARDOWN") { return Method::kTeardown; }
        return std::nullopt;
    }
};

}
