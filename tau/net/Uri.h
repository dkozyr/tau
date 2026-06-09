#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <optional> //TODO: <etl/optional.h>?
#include <cstdint>

namespace tau::net {

enum Protocol {
    kHttp,
    kHttps,
    kRtsp,
    kRtsps,
    kStun,
    kTurn,
};

enum Transport {
    kTcp,
    kUdp,
};

struct Uri {
    Protocol protocol;
    etl::string<32> host;
    uint16_t port;
    etl::string<256> path;
    std::optional<Transport> transport = std::nullopt;
};

std::optional<Uri> GetUriFromString(etl::string_view str);

}
