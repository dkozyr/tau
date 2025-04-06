#include "tau/net/Uri.h"
#include "tau/common/String.h"

#include "tau/common/Log.h"

namespace tau::net {

std::optional<Uri> GetStunUriFromString(std::string_view str);
std::optional<Uri> GetTurnUriFromString(std::string_view str);
bool ValidateHost(const std::string_view& host);
std::optional<uint16_t> ValidateAndParsePort(const std::string_view& port);

std::optional<Uri> GetUriFromString(std::string_view str) {
    if(IsPrefix(str, "stun:")) { return GetStunUriFromString(str); }
    if(IsPrefix(str, "turn:")) { return GetTurnUriFromString(str); }

    auto tokens = Split(str, "://");
    if(tokens.size() != 2) {
        return std::nullopt;
    }

    Protocol protocol;
    uint16_t port;
    if(tokens[0] == "http") {
        protocol = Protocol::kHttp;
        port = 80;
    } else if(tokens[0] == "https") {
        protocol = Protocol::kHttps;
        port = 443;
    } else if(tokens[0] == "rtsp") {
        protocol = Protocol::kRtsp;
        port = 554;
    } else if(tokens[0] == "rtsps") {
        protocol = Protocol::kRtsps;
        port = 554;
    } else {
        return std::nullopt;
    }

    auto& host_port_path = tokens[1];
    auto pos = host_port_path.find("/");
    if(pos == 0) {
        return std::nullopt;
    }
    const std::string path = (pos != std::string::npos) ? std::string{host_port_path.substr(pos + 1)} : std::string{};

    auto host_port = host_port_path.substr(0, pos);
    auto host_tokens = Split(host_port, ":");
    if(host_tokens.size() > 2) {
        return std::nullopt;
    }
    if(!ValidateHost(host_tokens[0])) {
        return std::nullopt;
    }
    if(host_tokens.size() == 2) {
        auto port_parsed = ValidateAndParsePort(host_tokens[1]);
        if(!port_parsed) {
            return std::nullopt;
        }
        port = *port_parsed;
    }
    return Uri{
        .protocol = protocol,
        .host = std::string{host_tokens[0]},
        .port = port,
        .path = path
    };
}

std::optional<Uri> GetStunUriFromString(std::string_view str) {
    uint16_t port = 3478;
    auto tokens = Split(str, ":");
    if(tokens.size() == 3) {
        auto port_parsed = ValidateAndParsePort(tokens[2]);
        if(!port_parsed) {
            return std::nullopt;
        }
        port = *port_parsed;
    } else if(tokens.size() > 3) {
        return std::nullopt;
    }
    if(!ValidateHost(tokens[1])) {
        return std::nullopt;
    }
    return Uri{
        .protocol = Protocol::kStun,
        .host = std::string{tokens[1]},
        .port = port,
        .path = {},
        .transport = Transport::kUdp
    };
}

std::optional<Uri> GetTurnUriFromString(std::string_view str) {
    uint16_t port = 3478;
    std::optional<Transport> transport;
    auto transport_tokens = Split(str, "?transport=");
    if(transport_tokens.size() == 2) {
        if(transport_tokens[1] == "udp")      { transport = Transport::kUdp; }
        else if(transport_tokens[1] == "tcp") { transport = Transport::kTcp; }
        else {
            return std::nullopt;
        }
        str = transport_tokens[0];
    } else if(transport_tokens.size() > 2) {
        return std::nullopt;
    }

    auto tokens = Split(str, ":");
    if(!ValidateHost(tokens[1])) {
        return std::nullopt;
    }
    if(tokens.size() == 3) {
        auto port_parsed = ValidateAndParsePort(tokens[2]);
        if(!port_parsed) {
            return std::nullopt;
        }
        port = *port_parsed;
    } else if(tokens.size() > 3) {
        return std::nullopt;
    }

    return Uri{
        .protocol = Protocol::kTurn,
        .host = std::string{tokens[1]},
        .port = port,
        .path = {},
        .transport = transport
    };
}

bool ValidateHost(const std::string_view& host) {
    bool ok = false;
    for(auto c : host) {
        ok = std::isalnum(c) || (c == '.') || (c == '-');
        if(!ok) {
            break;
        }
    }
    return ok;
}

std::optional<uint16_t> ValidateAndParsePort(const std::string_view& port) {
    for(auto c : port) { //TODO: remove it?
        if(!std::isalnum(c)) {
            return std::nullopt;
        }
    }
    auto parsed = StringToUnsigned<uint64_t>(port);
    if(!parsed || (*parsed > std::numeric_limits<uint16_t>::max())) {
        return std::nullopt;
    }
    return static_cast<uint16_t>(*parsed);
}

}
