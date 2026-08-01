#pragma once

#include "apps/signalling/SessionState.h"
#include "apps/signalling/message/Type.h"
#include "apps/signalling/message/Payload.h"
#include "tau/common/Json.h"
#include <optional>

namespace tau::signalling::message {

//TODO: rename to DeviceNotification?
struct DeviceNotification {
    StreamId stream_id;
    std::optional<SessionId> session_id = std::nullopt;
    std::optional<SessionState> session_state = std::nullopt;
    Payload payload = {};
};

void DeviceNotificationToJson(etl::string_stream& ss, const DeviceNotification& notification);
std::optional<DeviceNotification> DeviceNotificationFromJson(const Json::object& parsed);

etl::string_stream& operator<<(etl::string_stream& ss, const DeviceNotification& notification);

}
