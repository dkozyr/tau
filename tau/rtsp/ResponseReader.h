#pragma once

#include "tau/rtsp/Response.h"
#include "tau/common/String.h"
#include <optional>

namespace rtsp {

class ResponseReader {
public:
    static std::optional<Response> Read(std::string_view str) {
        auto lines = Split(str, kClRf);
        if(lines.size() < 2) {
            return std::nullopt;
        }
        auto tokens = Split(lines[0], " ");
        if((tokens.size() != 3) || (tokens[0] != kRtspVersion)) {
            return std::nullopt;
        }
        auto status_code = StringToUnsigned<size_t>(tokens[1]);
        auto cseq = StringToUnsigned<size_t>(GetHeaderValue(lines[1], "CSeq: "));
        if(!status_code || !cseq) {
            return std::nullopt;
        }

        const auto body_offset = str.find(kClRfClRf);
        return Response{
            .status_code = *status_code,
            .reason_phrase = std::string(tokens[2]),
            .cseq = *cseq,
            .headers = GetHeaders(lines),
            .body = (body_offset != std::string::npos)
                  ? std::string{str.substr(body_offset + kClRfClRf.size())}
                  : std::string{}
        };
    }
};

}
