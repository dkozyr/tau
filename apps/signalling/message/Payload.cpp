#include "apps/signalling/message/Payload.h"

namespace tau::signalling::message {

PayloadType PayloadTypeFromString(const etl::string_view& str) {
    if(str == "empty")          { return PayloadType::kEmpty; }
    if(str == "sdp")            { return PayloadType::kSdp; }
    if(str == "ice_candidates") { return PayloadType::kIceCandidates; }
    return PayloadType::kError;
}

etl::string_stream& operator<<(etl::string_stream& ss, const PayloadType& type) {
    switch(type) {
        case PayloadType::kEmpty:         ss << "empty"; break;
        case PayloadType::kSdp:           ss << "sdp"; break;
        case PayloadType::kIceCandidates: ss << "ice_candidates"; break;
        case PayloadType::kError:         ss << "error"; break;
    }
    return ss;
}

void PayloadToJson(etl::string_stream& ss, const Payload& payload) {
    ss << "{";
    ss << "\"type\":\"" << payload.type << "\"";
    ss << ",\"data\":";
    json::SerializeString(ss, payload.data);
    ss << "}";
}

std::optional<Payload> PayloadFromJson(const Json::object& parsed) {
    etl::string<16> type;
    json::GetString(parsed, "type", type);
    if(type.empty()) {
        return std::nullopt;
    }

    Payload payload{.type = PayloadTypeFromString(type)};
    json::GetString(parsed, "data", payload.data);
    return payload;
}

etl::string_stream& operator<<(etl::string_stream& ss, const Payload& payload) {
    ss << "{type: " << payload.type << ", data: ";
    if(payload.type != PayloadType::kSdp) {
        ss << payload.data;
    } else {
        ss << "[SDP]";
    }
    return ss << "}";
}

}
