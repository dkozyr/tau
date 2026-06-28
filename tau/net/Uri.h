#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <optional>
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
    using Host = etl::string<32>;
    using Path = etl::string<256>;

    Protocol protocol;
    Host host;
    uint16_t port;
    Path path;
    std::optional<Transport> transport = std::nullopt;
};

std::optional<Uri> GetUriFromString(etl::string_view str);

}
