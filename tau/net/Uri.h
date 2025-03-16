#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>

namespace tau::net {

enum Protocol {
    kHttp,
    kHttps,
    kRtsp,
    kRtsps,
};

struct Uri {
    Protocol protocol;
    std::string host;
    uint16_t port;
    std::string path;
};

std::optional<Uri> GetUriFromString(std::string_view str);

}
