#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <optional>
#include <cstdint>

namespace tau::rtsp {

enum class Transport {
    kUdp,
    kTcp
};

struct InterleavedChannels {
    uint8_t rtp = 0;
    uint8_t rtcp = 1;
};

struct TransportParameters {
    Transport transport = Transport::kUdp;
    std::optional<uint16_t> server_rtp_port;
    std::optional<InterleavedChannels> channels;
};

std::optional<TransportParameters> ParseTransport(etl::string_view value);

//TODO: stringstream?
etl::string<64> CreateTransportHeader(Transport transport, uint16_t rtp_port = 0);

}
