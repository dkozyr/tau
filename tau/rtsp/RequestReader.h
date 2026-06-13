#pragma once

#include "tau/rtsp/Request.h"
#include "tau/common/String.h"
#include <optional>

namespace tau::rtsp {

class RequestReader {
public:
    static std::optional<Request> Read(etl::string_view str) {
        SplitTokens<2> lines;
        Split(lines, str, kClRf);
        if(lines.size() < 2) {
            return std::nullopt;
        }
        SplitTokens<3> tokens;
        Split(tokens, lines[0], " ");
        if((tokens.size() != 3) || (tokens[2] != kRtspVersion)) {
            return std::nullopt;
        }
        auto method = GetMethod(tokens[0]);
        if(!method) {
            return std::nullopt;
        }
        auto headers = GetHeaders(str);
        if(GetHeaderValue(HeaderName::kCSeq, headers).empty()) {
            return std::nullopt;
        }
        return Request{
            .uri = tokens[1],
            .method = *method,
            .headers = std::move(headers)
        };
    }

private:
    static std::optional<Method> GetMethod(etl::string_view token) {
        if(token == "OPTIONS")  { return Method::kOptions; }
        if(token == "DESCRIBE") { return Method::kDescribe; }
        if(token == "SETUP")    { return Method::kSetup; }
        if(token == "PLAY")     { return Method::kPlay; }
        if(token == "TEARDOWN") { return Method::kTeardown; }
        return std::nullopt;
    }
};

}
