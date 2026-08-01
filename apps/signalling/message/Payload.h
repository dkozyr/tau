#pragma once

#include "apps/signalling/message/Type.h"
#include "tau/ws/Message.h"
#include "tau/common/Json.h"
#include <optional>

namespace tau::signalling::message {

enum class PayloadType {
    kEmpty,
    kSdp,
    kIceCandidates,
    kError
};

struct Payload {
    PayloadType type = PayloadType::kEmpty;
    ws::String data = {};
};

PayloadType PayloadTypeFromString(const etl::string_view& str);
etl::string_stream& operator<<(etl::string_stream& ss, const PayloadType& type);

void PayloadToJson(etl::string_stream& ss, const Payload& payload);
std::optional<Payload> PayloadFromJson(const Json::object& parsed);

etl::string_stream& operator<<(etl::string_stream& ss, const Payload& payload);

}
