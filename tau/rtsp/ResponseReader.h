#pragma once

#include "tau/rtsp/Response.h"
#include "tau/common/String.h"
#include <optional>

namespace tau::rtsp {

class ResponseReader {
public:
    static std::optional<Response> Read(etl::string_view str) {
        SplitTokens<2> lines;
        Split(lines, str, kClRf);
        if(lines.size() < 2) {
            return std::nullopt;
        }

        size_t position = 0;
        const auto version = SplitNext(lines[0], position, " ");
        if((version != kRtspVersion) || (position == etl::string_view::npos)) {
            return std::nullopt;
        }

        const auto status = SplitNext(lines[0], position, " ");
        const auto status_code = StringToUnsigned<size_t>(status);
        if((status.size() != 3) || !status_code) {
            return std::nullopt;
        }

        const auto reason_phrase = (position == etl::string_view::npos) ? etl::string_view{} : lines[0].substr(position);
        auto headers = GetHeaders(str);
        if(GetHeaderValue(HeaderName::kCSeq, headers).empty()) {
            return std::nullopt;
        }

        const auto body_offset = str.find(kClRfClRf);
        return Response{
            .status_code = *status_code,
            .reason_phrase = reason_phrase,
            .headers = std::move(headers),
            .body = (body_offset != etl::string_view::npos)
                  ? str.substr(body_offset + kClRfClRf.size())
                  : etl::string_view{}
        };
    }
};

}
