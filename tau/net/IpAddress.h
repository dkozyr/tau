#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace net {

inline const std::string kLocalHost = "127.0.0.1";

struct IpAddress {
    std::string address;
    std::optional<uint16_t> port = std::nullopt;
};

}
