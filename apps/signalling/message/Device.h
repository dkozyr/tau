#pragma once

#include "apps/signalling/SessionState.h"
#include "apps/signalling/message/Type.h"
#include "apps/signalling/message/Payload.h"
#include "tau/common/Json.h"
#include <optional>

namespace tau::signalling::message {

struct Device {
    Type type;
    DeviceId device_id;
    std::optional<SessionId> session_id = std::nullopt;
    Payload payload = {};
};

void DeviceToJson(etl::string_stream& ss, const Device& device);
std::optional<Device> DeviceFromJson(const Json::object& parsed);

etl::string_stream& operator<<(etl::string_stream& ss, const Device& device);

}
