#include "tau/rtsp/Transport.h"
#include "tau/common/String.h"

namespace tau::rtsp {

std::optional<std::pair<uint16_t, uint16_t>> ParsePortPair(etl::string_view value);

std::optional<TransportParameters> ParseTransport(etl::string_view value) {
    if(value.find(',') != etl::string_view::npos) {
        return std::nullopt;
    }
    size_t pos = 0;
    const auto protocol = Trim(SplitNext(value, pos, ";"));

    TransportParameters parameters;
    if(Equal(protocol, "RTP/AVP/TCP")) {
        parameters.transport = Transport::kTcp;
    } else if(!Equal(protocol, "RTP/AVP") && !Equal(protocol, "RTP/AVP/UDP")) {
        return std::nullopt;
    }

    while(pos != etl::string_view::npos) {
        const auto token = SplitNext(value, pos, ";");

        SplitTokens<3> parts;
        Split(parts, token, "=");
        if(parts.empty()) {
            continue;
        }

        const auto name = Trim(parts[0]);

        if(Equal(name, "interleaved")) {
            if(parameters.channels || (parameters.transport != Transport::kTcp)) {
                return std::nullopt;
            }
            if(parts.size() != 2) {
                return std::nullopt;
            }

            const auto rtp_rtcp = ParsePortPair(Trim(parts[1]));
            if(!rtp_rtcp || (rtp_rtcp->first > 255) || (rtp_rtcp->second > 255)) {
                return std::nullopt;
            }

            parameters.channels = InterleavedChannels{
                .rtp  = static_cast<uint8_t>(rtp_rtcp->first),
                .rtcp = static_cast<uint8_t>(rtp_rtcp->second)
            };
        } else if(Equal(name, "server_port")) {
            if(parameters.server_rtp_port || (parameters.transport != Transport::kUdp)) {
                return std::nullopt;
            }
            if(parts.size() != 2) {
                return std::nullopt;
            }

            const auto rtp_rtcp = ParsePortPair(Trim(parts[1]));
            if(!rtp_rtcp || (rtp_rtcp->first == 0) || (rtp_rtcp->second == 0) || (rtp_rtcp->first + 1 != rtp_rtcp->second)) {
                return std::nullopt;
            }

            parameters.server_rtp_port = rtp_rtcp->first;
        } else if(Equal(name, "multicast")) {
            return std::nullopt;
        }

    }
    if((parameters.transport == Transport::kTcp) ? !parameters.channels : !parameters.server_rtp_port) {
        return std::nullopt;
    }
    return parameters;
}

etl::string<64> CreateTransportHeader(Transport transport, uint16_t rtp_port) {
    if(transport == Transport::kTcp) {
        return etl::string<64>{"RTP/AVP/TCP;unicast;interleaved=0-1"};
    }
    etl::string<64> value{"RTP/AVP/UDP;unicast;client_port="};
    value.append(ToString<8>(rtp_port));
    value.append("-");
    value.append(ToString<8>(rtp_port + 1));
    return value;
}

std::optional<std::pair<uint16_t, uint16_t>> ParsePortPair(etl::string_view value) {
    SplitTokens<3> parts;
    Split(parts, value, "-");
    if(parts.size() != 2) {
        return std::nullopt;
    }

    const auto first  = Trim(parts[0]);
    const auto second = Trim(parts[1]);
    if((first.size() > 5) || (second.size() > 5)) {
        return std::nullopt;
    }
    const auto rtp  = StringToUnsigned<uint16_t>(first);
    const auto rtcp = StringToUnsigned<uint16_t>(second);
    if(!rtp || !rtcp || (*rtp == *rtcp)) {
        return std::nullopt;
    }
    return std::make_pair(*rtp, *rtcp);
}

}
